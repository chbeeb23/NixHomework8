// Copyright Epic Games, Inc. All Rights Reserved.

#include "NixHomework8Character.h"
#include "Engine/OverlapResult.h"
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
#include "Net/UnrealNetwork.h"
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

	bReplicates = true;
	SetReplicateMovement(true);
}

void ANixHomework8Character::BeginPlay()
{
	Super::BeginPlay();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &ANixHomework8Character::OnSecondAttackMontageEnded);
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
	Server_DoMove(MovementVector);
}

void ANixHomework8Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	Server_DoLook(LookAxisVector);
}

void ANixHomework8Character::LookCompleted(const FInputActionValue& Value)
{
	RightInputValue = 0;
}

void ANixHomework8Character::Attack(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		Server_Attack();
	}
}

void ANixHomework8Character::SecondAttack(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		Server_SecondAttack();
	}
}

void ANixHomework8Character::Server_ProcessAttack_Implementation()
{
	NetMulticast_ProcessAttack();
}

void ANixHomework8Character::NetMulticast_ProcessAttack_Implementation()
{
	bAttacking = false;
}

void ANixHomework8Character::Server_FinishAttack_Implementation()
{
	NetMulticast_FinishAttack();
}

void ANixHomework8Character::NetMulticast_FinishAttack_Implementation()
{
	ResetMovement();
}

void ANixHomework8Character::Server_HitDamage_Implementation(EAttackHand AttackHand)
{
	FName Socket = AttackHand == EAttackHand::Right ? RightAttackSocket : LeftAttackSocket;
	FVector SocketLocation = GetMesh()->GetSocketLocation(Socket);

	TArray<FOverlapResult> Results;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(100.f);
	GetWorld()->OverlapMultiByChannel(Results, SocketLocation, FQuat::Identity, ECC_Pawn, Sphere);

	for (const FOverlapResult& Result : Results)
	{
		ANixHomework8Character* Actor = Cast<ANixHomework8Character>(Result.GetActor());

		if (Actor)
		{
			Actor->TakeDamageInternal();
		}
	}

	NetMulticast_HitDamage(AttackHand);
}

void ANixHomework8Character::NetMulticast_HitDamage_Implementation(EAttackHand AttackHand)
{
	FName Socket = AttackHand == EAttackHand::Right ? RightAttackSocket : LeftAttackSocket;
	FVector SocketLocation = GetMesh()->GetSocketLocation(Socket);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), OnAttackParticle, SocketLocation, FRotator::ZeroRotator, FVector(0.2f));
}

void ANixHomework8Character::TakeDamage(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		Server_TakeDamage();
	}
}

void ANixHomework8Character::Server_TakeDamage_Implementation()
{
	TakeDamageInternal();
}

void ANixHomework8Character::TakeDamageInternal()
{
	if (bInDamagedState)
	{
		return;
	}

	if (!ensureMsgf(DamageAnim, TEXT("DamageAnim is not set on %s"), *GetName()))
	{
		return;
	}

	bDamaged = true;
	bInDamagedState = true;
	TakenDamage = FMath::RandRange(50, 100);
	float Duration = DamageAnim->GetPlayLength();
	GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &ANixHomework8Character::OnDamageProcessed, Duration / 2, false);
	GetWorldTimerManager().SetTimer(FinishDamageTimerHandle, this, &ANixHomework8Character::OnDamageFinished, Duration, false);

	NetMulticast_TakeDamage();
}

void ANixHomework8Character::NetMulticast_TakeDamage_Implementation()
{
	GEngine->AddOnScreenDebugMessage(2, 10.f, FColor::Cyan, FString::Printf(TEXT("Take damage: %d"), TakenDamage));
	GetCharacterMovement()->DisableMovement();
}

void ANixHomework8Character::OnDamageProcessed()
{
	bDamaged = false;
}

void ANixHomework8Character::OnDamageFinished()
{
	bInDamagedState = false;
	ResetMovement();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 5.f, FColor::Green, FString::Printf(TEXT("%s recovered"), *GetName()));
	}
}

void ANixHomework8Character::OnSecondAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != SecondAttackMontage)
	{
		return;
	}

	ResetMovement();
}

void ANixHomework8Character::ResetMovement()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ANixHomework8Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANixHomework8Character, ForwardInputValue);
	DOREPLIFETIME(ANixHomework8Character, RightInputValue);
	DOREPLIFETIME(ANixHomework8Character, bAttacking);
	DOREPLIFETIME(ANixHomework8Character, bDamaged);
	DOREPLIFETIME(ANixHomework8Character, TakenDamage);
	DOREPLIFETIME(ANixHomework8Character, bInDamagedState);
}

void ANixHomework8Character::Server_DoMove_Implementation(FVector2D MovementVector)
{
	ForwardInputValue = MovementVector.Y;
	RightInputValue = MovementVector.X;

	Client_DoMove(MovementVector);
}

void ANixHomework8Character::Client_DoMove_Implementation(FVector2D MovementVector)
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
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ANixHomework8Character::Server_DoLook_Implementation(FVector2D LookAxisVector)
{
	RightInputValue = LookAxisVector.X;

	Client_DoLook(LookAxisVector);
}

void ANixHomework8Character::Client_DoLook_Implementation(FVector2D LookAxisVector)
{
	GEngine->AddOnScreenDebugMessage(0, 10.f, FColor::Red, FString::Printf(TEXT("Move Y: %f"), ForwardInputValue));
	GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Green, FString::Printf(TEXT("Move X: %f"), RightInputValue));

	if (GetController() != nullptr)
	{
		if (RightInputValue != 1 && RightInputValue != -1)
		{
			RightInputValue = LookAxisVector.X;
		}

		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
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

void ANixHomework8Character::Server_Attack_Implementation()
{
	if (!ensureMsgf(AttackAnim, TEXT("AttackAnim is not set on %s"), *GetName()))
	{
		return;
	}

	bAttacking = true;
	float Duration = AttackAnim->GetPlayLength() / 2;
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ANixHomework8Character::Server_ProcessAttack, Duration, false);
	NetMulticast_Attack();
}

void ANixHomework8Character::NetMulticast_Attack_Implementation()
{
	GetCharacterMovement()->DisableMovement();
}

void ANixHomework8Character::Server_SecondAttack_Implementation()
{
	NetMulticast_SecondAttack();
}

void ANixHomework8Character::NetMulticast_SecondAttack_Implementation()
{
	if (!SecondAttackMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	GetCharacterMovement()->DisableMovement();
	AnimInstance->Montage_Play(SecondAttackMontage, 1.0f);
}