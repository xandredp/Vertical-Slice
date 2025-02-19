// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/SaveGame.h"

#include "Core/TypeClass.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayController.h"
#include "Assets/BatteryPickup.h"
//#include "GameFramework/PlayerController.h"

#include "SaveGameModule.generated.h" 


/**
 * 
 */

UCLASS()
class NO_BARK_VS_API USaveGameModule : public USaveGame
{
	GENERATED_BODY()
	
public:

	// Saved variables from NBCharacter

	UPROPERTY(EditAnywhere, Category = "Save")
		FVector PlayerPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		int CurrentScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		int KillCount;

	// Weapon stuff 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spawn)
		TSubclassOf <class ABaseWeapon> WeaponClass;
	//Hand Held Item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
	  bool HasWeapon;

	// Torch

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spawn)
		TSubclassOf <class ABaseTorch> TorchClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		bool HasTorch;


	// Try to save Array of Players
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	TArray<FString> Zombies;
	//

	// Playcontroller Battery data and Key daya
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
		TArray<FBatteryData> CurrentPossesedBatteries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widgets)
		FBatteryData CurrentPossesedBattery;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
		TArray<FKeyData> CurrentPossesedKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widgets)
		FKeyData CurrentPossesedKey;


	// This stuff belongs to the current weapon

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
		int32 CurrentClip;
	//weapon reloaded ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
		int32 CurrentAmmo;

	// This stuff actually comes from the Player Controller Blueprint 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
		int32 Hours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
		int32 Minutes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
		int32 Seconds;


	UPROPERTY(EditAnywhere, Category = "Basic")
		FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
		uint32 UserIndex;

	USaveGameModule();
	
	UFUNCTION(BlueprintCallable, Category = "Config")
		void SaveGame(ACharacter * ThisCharacter);

	UFUNCTION(BlueprintCallable, Category = "Config")
		void LoadGame(ACharacter * ThisCharacter);
	
};
