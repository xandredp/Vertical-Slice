// Fill out your copyright notice in the Description page of Project Settings.
#include "Core/BaseWeapon.h"
#include "Core/No_Bark_Vs.h"
#include "Player/NBCharacter.h"
#include "Player/PlayController.h"
#include "GameFramework/Actor.h"
#include "Items/BaseImpactEffect.h"
#include "NBDamageType.h"
#include "Monsters/Base/NBBaseAI.h"

ABaseWeapon::ABaseWeapon()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
//	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetVisibility(true);
	WeaponMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	WeaponMesh->bReceivesDecals = true;
	WeaponMesh->CastShadow = true;
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	WeaponCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionComp"));
	WeaponCollisionComp->SetupAttachment(WeaponMesh);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	WeaponConfig.WeaponDamage = 20;
	WeaponConfig.WeaponSplits = 5;
	WeaponConfig.MaxAmmo = 5;
	WeaponConfig.MaxClip = 5;
	CurrentState = EWeaponState::Idle;
	TrailTargetParam = "EndPoint";
	MuzzleAttachPoint = "MuzzleTip";

	EquipAnimDuration = 0.5f;
	FireAnimDuration = 1.5f;
	ReloadAnimDuration = 1.1f;
	WeaponConfig.TimeBetweenShots = 0.1f;

	MaxUseDistance =600;
	CurrentUseDistance = MaxUseDistance;
	PushRange = 100;
}

class ANBCharacter* ABaseWeapon::GetPawnOwner() const
{
	return MyPawn;
}

void ABaseWeapon::SetTimerForFiring()
{
	Fire();
}
void ABaseWeapon::StopTimerForFiring()
{
	GetWorldTimerManager().ClearTimer(FiringTimerHandle);
}
void ABaseWeapon::FireBullets()
{
	if (CurrentAmmo > 0)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Black, TEXT("Bullet"));
		Instant_Fire();
	//	MakeNoise(0.5, GetPawnOwner(), GetActorLocation());

		CurrentAmmo -= WeaponConfig.ShotCost;
	}
	else
	{
		ReloadAmmo();
	}
}
void ABaseWeapon::Fire()
{
	if (ProjectileType == EProjectileType::EBullet)
	{
		FireBullets();
		GetWorldTimerManager().SetTimer(FiringTimerHandle, this, &ABaseWeapon::FireBullets, WeaponConfig.TimeBetweenShots, true);
	}
	if (ProjectileType == EProjectileType::ESpread)
	{
		if (CurrentAmmo > 0)
		{
			for (int32 i = 0; i <= WeaponConfig.WeaponSplits; i++)
			{
				Instant_Fire();
			}
			CurrentAmmo -= WeaponConfig.ShotCost;
			// Signal a gunshot
		//	MakeNoise(0.5, GetPawnOwner(), GetActorLocation());
		}
		else
		{
			ReloadAmmo();
		}
	}
	if (ProjectileType == EProjectileType::EProjectile)
	{
		if (CurrentAmmo > 0)
		{
			
		//	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Black, TEXT("Projectile"));
			ProjectileFire();

			CurrentAmmo -= WeaponConfig.ShotCost;

			// Signal a gunshot
		MakeNoise(0.5, GetPawnOwner(), GetActorLocation());
		}
		else
		{
			ReloadAmmo();
		}
	}
}
void ABaseWeapon::Instant_Fire()
{
	SimulateWeaponFire();

	const int32 RandomSeed = FMath::Rand();
	FRandomStream WeaponRandomStream(RandomSeed);
	const float CurrentSpread = WeaponConfig.WeaponSpread;
	const float SpreadCone = FMath::DegreesToRadians(WeaponConfig.WeaponSpread * 0.5);
	const FVector MuzzleOrigin = WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
	const FVector AimDir = GetAdjustedAim();
	const FVector CameraPos = GetCameraDamageStartLocation(AimDir);
	
	FVector AdjustedAimDir = WeaponRandomStream.VRandCone(AimDir, SpreadCone, SpreadCone);

	const FVector EndPos = CameraPos + (AdjustedAimDir *  WeaponConfig.WeaponRange);

		/* Check for impact by tracing from the camera position */
	FHitResult Impact = WeaponTrace(CameraPos, EndPos);

	//const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, SpreadCone, SpreadCone);
	if (Impact.bBlockingHit)
	{
		/* Adjust the shoot direction to hit at the crosshair. */
		AdjustedAimDir = (Impact.ImpactPoint - MuzzleOrigin).GetSafeNormal();
		/* Re-trace with the new aim direction coming out of the weapon muzzle */
		Impact = WeaponTrace(MuzzleOrigin, MuzzleOrigin + (AdjustedAimDir *  WeaponConfig.WeaponRange));
	}
	else
	{
		/* Use the maximum distance as the adjust direction */
		Impact.ImpactPoint = FVector_NetQuantize(EndPos);
	}

	ProcessInstantHit(Impact, MuzzleOrigin, AdjustedAimDir, RandomSeed, CurrentSpread);
}

void ABaseWeapon::PushEnemy()
{
	const FVector AimDir = GetAdjustedAim();
	const FVector CameraPos = GetCameraDamageStartLocation(AimDir);

	const FVector EndPos = CameraPos + (AimDir * PushRange);

	/* Check for impact by tracing from the camera position */
	FHitResult Impact = WeaponTrace(CameraPos, EndPos);
}

FVector ABaseWeapon::GetAdjustedAim() const
{
	APlayController* const PC = Instigator ? Cast<APlayController>(Instigator->Controller) : nullptr;
	FVector FinalAim = FVector::ZeroVector;

	if (PC)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}


FVector ABaseWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	APlayController* PC = MyPawn ? Cast<APlayController>(MyPawn->Controller) : nullptr;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		FRotator DummyRot;
		PC->GetPlayerViewPoint(OutStartTrace, DummyRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * (FVector::DotProduct((Instigator->GetActorLocation() - OutStartTrace), AimDir));
	}

	return OutStartTrace;
}
void ABaseWeapon::ProjectileFire()
{
}

FHitResult ABaseWeapon::WeaponTrace(const FVector & TraceFrom, const FVector & TraceTo) const
{

	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;
	//TraceParams.AddIgnoredActor(this);

	FHitResult Hit(ForceInit);

	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, WEAPON_TRACE, TraceParams);
	//DrawDebugLine(GetWorld(), TraceFrom, TraceTo, FColor::Green, false, 10.0, 0, 0.5f);

	return Hit;
}

void ABaseWeapon::ProcessInstantHit(const FHitResult & Impact, const FVector & Origin, const FVector & ShootDir, int32 RandomSeed, float ReticleSpread)
{
	float ActualHitDamage = WeaponConfig.WeaponDamage;
	UPhysicalMaterial * PhysMat = Impact.PhysMaterial.Get();
	
	ANBBaseAI *Enemy = Cast<ANBBaseAI>(Impact.GetActor());

	float Damage = 10.0f;

	APlayController* const PC = Instigator ? Cast<APlayController>(Instigator->Controller) : nullptr;

	PC->ShotsFired++;
	MyPawn->ShotsFiredEvent();

	float CurrentDamage = 0;
//	if (PhysMat && DmgType)
	if (Enemy!= nullptr)
	{	
		ANBBaseAI *BaseAI = Cast<ANBBaseAI>(Impact.GetActor());
			
		if (PhysMat)
		{
			if (PhysMat->SurfaceType == SURFACE_ENEMYHEAD)
			{

				CurrentDamage = WeaponConfig.WeaponDamage * 2.0f;
			//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "YOU HIT A Head!!");
				Enemy->ApplyDamage(Enemy, Damage, Origin, Impact, PC, this, DamageType);//TSubclassOf<UDamageType> DamageTypeClass)
				Enemy->ReduceHealth(CurrentDamage);
				Enemy->OnShotAt();
			}
			else if (PhysMat->SurfaceType == SURFACE_ENEMYLIMB)
			{
				CurrentDamage = WeaponConfig.WeaponDamage* 0.5f;
			//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "YOU HIT A Limb!!");
				Enemy->ApplyDamage(Enemy, Damage, Origin, Impact, PC, this, DamageType);//TSubclassOf<UDamageType> DamageTypeClass)
				Enemy->ReduceHealth(CurrentDamage);
				Enemy->OnShotAt();
			}
			else if (PhysMat->SurfaceType == SURFACE_ENEMYBODY)
			{
				CurrentDamage = WeaponConfig.WeaponDamage;
			//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "YOU HIT A BODY!!");
				Enemy->ApplyDamage(Enemy, Damage, Origin, Impact, PC, this, DamageType);//TSubclassOf<UDamageType> DamageTypeClass)
				Enemy->ReduceHealth(CurrentDamage);
				Enemy->OnShotAt();
			}
			else if (PhysMat->SurfaceType == SURFACE_FLESH)
			{

				//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "YOU HIT A FLESH!!");
			}
			else if (PhysMat->SurfaceType == SURFACE_DEFAULT)
			{

				//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "YOU HIT A DEFAULT!!");
			}
			else
			{

			}
		}
	//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::SanitizeFloat(Enemy->Health));
	}
		VisualInstantHit(Impact.ImpactPoint);
}

void ABaseWeapon::SetOwningPawn(ANBCharacter * NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;
	}
}

void ABaseWeapon::AttachToPlayer()
{
	if (MyPawn)
	{
		DetachFromPlayer();

		USkeletalMeshComponent *Character = MyPawn->GetMesh();
		WeaponMesh->SetHiddenInGame(false);
		WeaponMesh->SetupAttachment(Character, "Weapon_Socket");
	}
}

void ABaseWeapon::DetachFromPlayer()
{
	WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	WeaponMesh->SetHiddenInGame(true);
}

void ABaseWeapon::OnEquip()
{
	WeaponCollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToPlayer();
}

void ABaseWeapon::OnUnEquip()
{
	DetachFromPlayer();
}

void ABaseWeapon::ReloadAmmo()
{
	if (CurrentAmmo < WeaponConfig.MaxAmmo)
	{
		if (CurrentClip > 0)
		{
			//playannimation
	/*		if (ReloadAnimation)
			{
				float AnimDuration = PlayWeaponAnimation(ReloadAnimation);
				if (AnimDuration <= 0.0f)
				{
					AnimDuration = ReloadAnimDuration;
				}
				GetWorldTimerManager().SetTimer(RelaodingTimerHandle, this, &ABaseWeapon::StopReloading, AnimDuration, false);
			}*/

			PlayWeaponSound(ReloadSound);

			// fill up the current ammo
			int32 AmmoRemainedInCurrentClip = CurrentAmmo;
			int32 AddedClipNumbers;
			int32 ResultClip = CurrentClip + AmmoRemainedInCurrentClip;
			// if there are plenty clips in my gun
			if (ResultClip > WeaponConfig.MaxAmmo)
			{
				AddedClipNumbers = WeaponConfig.MaxAmmo - CurrentAmmo; 
				CurrentAmmo = WeaponConfig.MaxAmmo;
			}
			// if there are no plenty clips in my gun. add all clips into gun
			else
			{
				CurrentAmmo = CurrentClip + AmmoRemainedInCurrentClip;
				AddedClipNumbers = CurrentClip; 
			}

			CurrentClip = CurrentClip - AddedClipNumbers;
		}
		
	}
	
}
void ABaseWeapon::StopReloading()
{
	GetWorldTimerManager().ClearTimer(RelaodingTimerHandle);
	/*StopWeaponAnimation(ReloadAnimation);*/
}

void ABaseWeapon::SimulateWeaponFire()
{
	if (MuzzleFX)
	{
	
		MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
		MuzzlePSC->AddWorldRotation(FRotator(0.0f, 90.0f, 0.0f));
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "FXFXSpwned..");
	}

	/*if (!bPlayingFireAnim)
	{
		PlayWeaponAnimation(FireAnimation);
		bPlayingFireAnim = true;
	}*/

	PlayWeaponSound(FireSound);
}

void ABaseWeapon::StopSimulatingWeaponFire()
{
	if (bPlayingFireAnim)
	{
		//StopWeaponAnimation(FireAnimation);
		bPlayingFireAnim = false;
	}
}

FVector ABaseWeapon::GetMuzzleLocation() const
{
	return FVector();
}

FVector ABaseWeapon::GetMuzzleDirection() const
{
	return FVector();
}

UAudioComponent * ABaseWeapon::PlayWeaponSound(USoundCue * Sound)
{
	UAudioComponent* WeaponAC = nullptr;

	if (Sound && MyPawn)
	{
		WeaponAC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
		// Signal a gunshot
		MakeNoise(0.0, GetPawnOwner(), GetActorLocation());

	}
	return WeaponAC;
}

float ABaseWeapon::PlayWeaponAnimation(UAnimMontage * Animation, float InPlayRate, FName StartSectionName)
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		if (Animation)
		{
			Duration = MyPawn->PlayAnimMontage(Animation, InPlayRate, StartSectionName);
		}
	}

	return Duration;
}

void ABaseWeapon::StopWeaponAnimation(UAnimMontage * Animation)
{
	if (MyPawn)
	{
		if (Animation)
		{
			MyPawn->StopAnimMontage(Animation);
		}
	}
}

void ABaseWeapon::VisualInstantHit(const FVector& ImpactPoint)
{
	/* Adjust direction based on desired crosshair impact point and muzzle location */
	const FVector MuzzleOrigin = WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
	const FVector AimDir = (ImpactPoint - MuzzleOrigin).GetSafeNormal();

	const FVector EndTrace = MuzzleOrigin + (AimDir *WeaponConfig.WeaponRange);
	const FHitResult Impact = WeaponTrace(MuzzleOrigin, EndTrace);

	if (Impact.bBlockingHit)
	{
	//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "IF Visual InstantHit!");

			VisualImpactEffects(Impact);
			//VisualTrailEffects(Impact.ImpactPoint);
	}
	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "ELSE Visual InstantHit!");
	//	VisualTrailEffects(EndTrace);
	}
}


void ABaseWeapon::VisualImpactEffects(const FHitResult& Impact)
{
	
	
	if (ImpactTemplate && Impact.bBlockingHit)
	{
	//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "IF Visual InstantHit!");
		// TODO: Possible re-trace to get hit component that is lost during replication.

		/* This function prepares an actor to spawn, but requires another call to finish the actual spawn progress. This allows manipulation of properties before entering into the level */
		ABaseImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<ABaseImpactEffect>(ImpactTemplate, FTransform(Impact.ImpactPoint.Rotation(), Impact.ImpactPoint));
		ANBBaseAI *Enemy = Cast<ANBBaseAI>(Impact.GetActor());
		if (Enemy)
		{
			EffectActor->IsEnemyInDark = false;
	/*		if (Enemy->bisMonsterInLight)
			{
				if (EffectActor)
				{
					EffectActor->IsEnemyInDark = false;
				}			
			}
			else
			{
				if (EffectActor)
				{
					EffectActor->IsEnemyInDark = true;
				}

			}*/
		}

		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint));
		}
	}
}

void ABaseWeapon::VisualImpactEffectsInDark(const FHitResult& Impact)
{

	if (InDarkmpactTemplate && Impact.bBlockingHit)
	{
		//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "IF Visual InstantHit!");
		// TODO: Possible re-trace to get hit component that is lost during replication.

		/* This function prepares an actor to spawn, but requires another call to finish the actual spawn progress. This allows manipulation of properties before entering into the level */
		ABaseImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<ABaseImpactEffect>(InDarkmpactTemplate, FTransform(Impact.ImpactPoint.Rotation(), Impact.ImpactPoint));
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint));
		}
	}
}

void ABaseWeapon::VisualTrailEffects(const FVector& EndPoint)
{
	const FVector MuzzleOrigin = WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
//	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "TRAILEFFECTWORKING!");
	// Keep local count for effects
	BulletsShotCount++;
	FVector ShootDir = EndPoint - MuzzleOrigin;

	// Only spawn if a minimum distance is satisfied.
	if (ShootDir.Size() < MinimumProjectileSpawnDistance)
	{
		return;
	}

	if (BulletsShotCount % TracerRoundInterval == 0)
	{
		if (TracerFX)
		{
			ShootDir.Normalize();
			UGameplayStatics::SpawnEmitterAtLocation(this, TracerFX, MuzzleOrigin, ShootDir.Rotation());
		}
	}
	else
	{
		// Only create trails FX by other players.
		ANBCharacter* OwningPawn = GetPawnOwner();
		if (OwningPawn && OwningPawn->IsLocallyControlled())
		{
			return;
		}

		if (TrailFX)
		{
			UParticleSystemComponent* TrailPSC = UGameplayStatics::SpawnEmitterAtLocation(this, TrailFX, MuzzleOrigin);
			if (TrailPSC)
			{
				TrailPSC->SetVectorParameter(TrailTargetParam, EndPoint);
			}
		}
	}
}

