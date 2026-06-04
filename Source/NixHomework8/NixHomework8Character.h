// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "NixHomework8Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UNiagaraSystem;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ANixHomework8Character : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AttackAction;

	/** Second Attack Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SecondAttackAction;

	/** Take Damage Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* TakeDamageAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* AttackAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* DamageAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SecondAttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UNiagaraSystem* OnAttackParticle;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	FName AttackSocket;

	virtual void BeginPlay() override;

public:

	/** Constructor */
	ANixHomework8Character();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void LookCompleted(const FInputActionValue& Value);

	void Attack(const FInputActionValue& Value);

	void SecondAttack(const FInputActionValue& Value);

	void TakeDamage(const FInputActionValue& Value);

	UFUNCTION()
	void OnSecondAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void ResetMovement();

public:

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Input")
	virtual void Server_DoMove(FVector2D MovementVector);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Input")
	virtual void Client_DoMove(FVector2D MovementVector);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Input")
	virtual void Server_DoLook(FVector2D LookAxisVector);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Input")
	virtual void Client_DoLook(FVector2D LookAxisVector);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(Server, Reliable, Category = "Input")
	virtual void Server_Attack(bool Value);

	UFUNCTION(NetMulticast, Reliable, Category = "Input")
	virtual void NetMulticast_Attack(bool Value);

	UFUNCTION(Server, Reliable, Category = "Input")
	virtual void Server_SecondAttack(bool Value);

	UFUNCTION(NetMulticast, Reliable, Category = "Input")
	virtual void NetMulticast_SecondAttack(bool Value);

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(Server, Reliable, Category = "Actions")
	void Server_ProcessAttack();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Actions")
	void NetMulticast_ProcessAttack();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Actions")
	void Server_FinishAttack();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Actions")
	void Client_FinishAttack();

	UFUNCTION(BlueprintCallable)
	void HitDamage();

	UFUNCTION(Server, Reliable, Category = "Actions")
	void Server_TakeDamage();

	UFUNCTION(NetMulticast, Reliable, Category = "Actions")
	void NetMulticast_TakeDamage();

	UFUNCTION(Server, Reliable, Category = "Actions")
	void Server_ProcessDamage();

	UFUNCTION(NetMulticast, Reliable, Category = "Actions")
	void NetMulticast_ProcessDamage();

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void FinishTakeDamage();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	float ForwardInputValue;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	float RightInputValue;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	bool bAttacking = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	bool bDamaged = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	int TakenDamage;

private:
	UPROPERTY(Replicated)
	bool bInDamagedState = false;

	FTimerHandle AttackTimerHandle;
	FTimerHandle DamageTimerHandle;
};

