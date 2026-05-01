// Copyright Epic Games, Inc. All Rights Reserved.

#include "NixHomework8Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "NixHomework8.h"


ANixHomework8Character::ANixHomework8Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ANixHomework8Character::BeginPlay()
{
	Super::BeginPlay();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.AddDynamic(
			this,
			&ANixHomework8Character::OnSecondAttackMontageEnded
		);
	}
}

void ANixHomework8Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANixHomework8Character::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ANixHomework8Character::Move);

		// Attack
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ANixHomework8Character::Attack);
		EnhancedInputComponent->BindAction(SecondAttackAction, ETriggerEvent::Triggered, this, &ANixHomework8Character::SecondAttack);

		// Taking damage
		EnhancedInputComponent->BindAction(TakeDamageAction, ETriggerEvent::Triggered, this, &ANixHomework8Character::TakeDamage);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANixHomework8Character::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &ANixHomework8Character::LookCompleted);
	}
	else
	{
		UE_LOG(LogNixHomework8, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANixHomework8Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ANixHomework8Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ANixHomework8Character::LookCompleted(const FInputActionValue& Value)
{
	RightInputValue = 0;
}

void ANixHomework8Character::Attack(const FInputActionValue& Value)
{
	bAttacking = Value.Get<bool>();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && bAttacking)
	{
		PC->SetIgnoreMoveInput(true);
	}
}

void ANixHomework8Character::SecondAttack(const FInputActionValue& Value)
{
	if (!Value.Get<bool>())
	{
		return;
	}

	if (!SecondAttackMontage) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
	}

	AnimInstance->Montage_Play(SecondAttackMontage, 1.0f);
}

void ANixHomework8Character::StartAttack()
{
	bAttacking = false;
}

void ANixHomework8Character::FinishAttack()
{
	ResetMovement();
}

void ANixHomework8Character::HitDamage()
{
	FVector SocketLocation = GetMesh()->GetSocketLocation(AttackSocket);

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		OnAttackParticle,
		SocketLocation,
		FRotator::ZeroRotator,
		FVector(0.2f)
	);
}

void ANixHomework8Character::TakeDamage(const FInputActionValue& Value)
{
	if (!bInDamagedState && Value.Get<bool>())
	{
		bDamaged = true;
		bInDamagedState = true;
		TakenDamage = FMath::RandRange(0, 100);
		GEngine->AddOnScreenDebugMessage(2, 10.f, FColor::Cyan, FString::Printf(TEXT("Take damage: %d"), TakenDamage));
	}
}

void ANixHomework8Character::StartTakeDamage()
{
	bDamaged = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
	}
}

void ANixHomework8Character::FinishTakeDamage()
{
	bInDamagedState = false;
	ResetMovement();
}

void ANixHomework8Character::OnSecondAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != SecondAttackMontage)
	{
		return;
	}

	FinishAttack();
}

void ANixHomework8Character::ResetMovement()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->ResetIgnoreMoveInput();
	}
}

void ANixHomework8Character::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);

		ForwardInputValue = Forward;
		RightInputValue = Right;
	}
}

void ANixHomework8Character::DoLook(float Yaw, float Pitch)
{
	GEngine->AddOnScreenDebugMessage(0, 10.f, FColor::Red, FString::Printf(TEXT("Move Y: %f"), ForwardInputValue));
	GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Green, FString::Printf(TEXT("Move X: %f"), RightInputValue));

	if (GetController() != nullptr)
	{
		if (RightInputValue != 1 && RightInputValue != -1)
		{
			RightInputValue = Yaw;
		}

		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ANixHomework8Character::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ANixHomework8Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
