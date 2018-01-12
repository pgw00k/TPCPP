// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TPCPPCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine.h"
#include "Kismet/KismetMathLibrary.h"

//////////////////////////////////////////////////////////////////////////
// ATPCPPCharacter

ATPCPPCharacter::ATPCPPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	// Set boost default value
	BoostDistance = 600;
	BoostSpeed = 20;
	BoostTime = 1;
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATPCPPCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATPCPPCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPCPPCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ATPCPPCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ATPCPPCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ATPCPPCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ATPCPPCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ATPCPPCharacter::OnResetVR);

	// My AdvanceControl
	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &ATPCPPCharacter::BoostStart);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ATPCPPCharacter::AimStart);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ATPCPPCharacter::AimEnd);
}


void ATPCPPCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ATPCPPCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ATPCPPCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ATPCPPCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ATPCPPCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ATPCPPCharacter::AddControllerYawInput(float Value)
{
	if (!isAiming)
	{
		APawn::AddControllerYawInput(Value);
	}
}

void ATPCPPCharacter::AddControllerPitchInput(float Value)
{
	if (!isAiming)
	{
		APawn::AddControllerPitchInput(Value);
	}
}

void ATPCPPCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f)&& !isBoosting)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATPCPPCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) && !isBoosting)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ATPCPPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (isBoosting) 
	{
		FHitResult HitResult;
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), BoostEndTarget, DeltaTime, BoostSpeed),true,&HitResult);
		if (HitResult.IsValidBlockingHit())
		{
			isBoosting = false;
		}
		BoostEnd();
	}

	if (isAiming)
	{
		Aim();
	}
}

void ATPCPPCharacter::BoostStart()
{
	if (isBoosting == false)
	{
		isBoosting = true;
		BoostEndTarget = GetActorForwardVector()*BoostDistance + GetActorLocation();
		BoostEndTarget.Z = GetActorForwardVector().Z + GetActorLocation().Z;

		//BoostEndTarget = FVector::FVector(
		//	GetActorForwardVector().X*BoostDistance + GetActorLocation().X,
		//	GetActorForwardVector().Y*BoostDistance + GetActorLocation().Y,
		//	GetActorForwardVector().Z + GetActorLocation().Z
		//);

	}
}

void ATPCPPCharacter::Boosting(float Deltatime)
{
	
	
}

void ATPCPPCharacter::BoostEnd()
{
	if (FVector::Distance(GetActorLocation(), BoostEndTarget) < 50 && isBoosting)
	{
		isBoosting = false;
	}

}

void ATPCPPCharacter::AimStart()
{
	isAiming = true;
	//FollowCamera->bUsePawnControlRotation = true;
}

void ATPCPPCharacter::AimEnd()
{
	isAiming = false;
	FollowCamera->bUsePawnControlRotation = false;
}

void ATPCPPCharacter::Aim()
{
	if (IsValid(Target))
	{
		FRotator Rot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target->GetActorLocation());
		Controller->SetControlRotation(Rot+CameraOffset);
		Rot.Pitch = 0;
		SetActorRelativeRotation(Rot);
		
	}
}
