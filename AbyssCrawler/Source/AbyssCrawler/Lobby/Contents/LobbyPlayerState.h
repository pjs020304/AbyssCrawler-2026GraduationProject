// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnLobbyColorChanged);
/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnDestroyed_Event(AActor* DestroyedActor);

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION(Client, Reliable, BlueprintCallable)
	void Client_KickedByServer();

public:
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void Multicast_Ready();

public:
	UFUNCTION(BlueprintCallable)
	void BP_RefreshLobbyUI();

	UFUNCTION(BlueprintCallable)
	void RefreshLobbyUI();

private:
	UFUNCTION()
	void OnRep_NicknameChange();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool Ready;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_NicknameChange)
	FText Nickname;

	// color
	UPROPERTY(ReplicatedUsing = OnRep_PlayerColorIndex, BlueprintReadOnly, Category = "Lobby|Color")
	int32 PlayerColorIndex = 0;

	FOnLobbyColorChanged OnLobbyColorChanged;

	void AddPlayerColorIndex(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Lobby|Color")
	FLinearColor GetPlayerColor() const;

	void SetPlayerColorIndex(int32 NewIndex);

	UFUNCTION()
	void OnRep_PlayerColorIndex();
};
