// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/TextureRenderTarget2D.h"

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PicItemWidget.generated.h"

/**
 * 
 */
#pragma pack(push,1)

struct FTGAFileHeader
{
	uint8 IdFieldLength;
	uint8 ColorMapType;
	uint8 ImageTypeCode;		// 2 for uncompressed RGB format
	uint16 ColorMapOrigin;
	uint16 ColorMapLength;
	uint8 ColorMapEntrySize;
	uint16 XOrigin;
	uint16 YOrigin;
	uint16 Width;
	uint16 Height;
	uint8 BitsPerPixel;
	uint8 ImageDescriptor;
	friend FArchive& operator<<(FArchive& Ar, FTGAFileHeader& H)
	{
		Ar << H.IdFieldLength << H.ColorMapType << H.ImageTypeCode;
		Ar << H.ColorMapOrigin << H.ColorMapLength << H.ColorMapEntrySize;
		Ar << H.XOrigin << H.YOrigin << H.Width << H.Height << H.BitsPerPixel;
		Ar << H.ImageDescriptor;
		return Ar;
	}
};
#pragma pack(pop)

struct FTGAFileFooter
{
	uint32 ExtensionAreaOffset;
	uint32 DeveloperDirectoryOffset;
	uint8 Signature[16];
	uint8 TrailingPeriod;
	uint8 NullTerminator;
};

UCLASS(Blueprintable, BlueprintType)
class FACETEXTUREMAKER_API UPicItemWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPicItemWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Desktop")
		void OpenDialog(TArray<FString>& outFiles);

	UFUNCTION(BlueprintCallable, Category = "Desktop")
		void SaveDialog(FString& path, const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "PicItem")
		UTexture2D* LoadTexture(FString path);

	UFUNCTION(BlueprintCallable, Category = "PicItem", meta = (DisplayName = "ExportRenderTarget2D"))
		bool ExportRenderTarget2D(UTextureRenderTarget2D* TexRT, const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "PicItem", meta = (DisplayName = "ExportTexture2D"))
		bool ExportTexture2D(UTexture2D* Tex2D);


private:
	bool ExportTexture2DWithDialog(UTexture2D* ObjectToExport);
	bool ExportToTGA(UTexture2D* ObjectToExport, const FString& FileName);
	bool ExportToPNG(UTexture2D* ObjectToExport, const FString& FileName);

private:
	FString PreOpenPath = TEXT("");
	FString PreSavePath = TEXT("");
};
