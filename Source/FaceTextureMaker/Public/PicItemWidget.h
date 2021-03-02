// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/TextureRenderTarget2D.h"

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PicItemWidget.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class FACETEXTUREMAKER_API UPicItemWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Desktop")
		void OpenDialog(TArray<FString>& outFiles);

	UFUNCTION(BlueprintCallable, Category = "Desktop")
		void SaveDialog(FString& path);

	UFUNCTION(BlueprintCallable, Category = "PicItem")
		UTexture2D* LoadTexture(FString path);

	UFUNCTION(BlueprintCallable, Category = "PicItem")
		bool ExportRenderTarget2DToPNG(UTextureRenderTarget2D* TexRT, const FString& FilePath, const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "PicItem")
		bool ExportRenderTexture2D(UTexture2D* Tex2D, const FString& FilePath, const FString& FileName);
};
