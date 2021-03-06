// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPCPPCharacter.generated.h"

UCLASS(config=Game)
class ATPCPPCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	ATPCPPCharacter();

	void ATPCPPCharacter::AddControllerYawInput(float Value);
	void ATPCPPCharacter::AddControllerPitchInput(float Value);

	void ATPCPPCharacter::Tick(float DeltaTime);
	void BoostStart();
	void Boosting(float Deltatime);
	void BoostEnd();

	void ATPCPPCharacter::AimStart();
	void ATPCPPCharacter::AimEnd();
	void ATPCPPCharacter::Aim();
	

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;


	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = CharacterBoost)
		bool isBoosting;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterBoost)
		FVector BoostEndTarget;

	UPROPERTY(EditAnywhere, Category = CharacterBoost)
		float BoostSpeed;

	UPROPERTY(EditAnywhere, Category = CharacterBoost)
		float BoostTime;

	UPROPERTY(EditAnywhere, Category = CharacterBoost)
		float BoostDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterAim)
		bool isAiming;
	UPROPERTY(EditAnywhere, Category = CharacterAim)
		class AActor* Target;

	UPROPERTY(EditAnywhere, Category = CharacterAim)
		FRotator CameraOffset;



protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

