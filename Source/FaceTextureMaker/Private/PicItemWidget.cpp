// Fill out your copyright notice in the Description page of Project Settings.

#include "PicItemWidget.h"

#include "Factories/Factory.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "AssetToolsModule.h"
#include "Paths.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "FileHelper.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "RendererUtils.h"

void UPicItemWidget::OpenDialog(TArray<FString>& outFiles)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	bool bOpened = false;

	FText PathError;
	FPaths::ValidatePath(PreOpenPath, &PathError);
	if (!PathError.IsEmpty())
		PreOpenPath = FPlatformProcess::UserDir();

	if (DesktopPlatform != NULL)
	{
		const FString FileTypes = TEXT("Texture (*.png;*.bmp;*.jpg;*.jpeg)|*.png;*.bmp;*.jpg;*.jpeg");
		bOpened = DesktopPlatform->OpenFileDialog
		(
			NULL,
			TEXT("Switch Textures"),
			PreOpenPath,
			TEXT(""),
			FileTypes,
			EFileDialogFlags::Multiple,
			outFiles
		);
	}

	if (!bOpened)
	{
		outFiles = TArray<FString>();
	}
	else
	{
		FString FileName, FileExt;
		FPaths::Split(outFiles[0], PreOpenPath, FileName, FileExt);
	}
}

void UPicItemWidget::SaveDialog(FString& path, const FString& FileName)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	bool bSave = false;
	
	FText PathError;
	FPaths::ValidatePath(PreSavePath, &PathError);
	if (!PathError.IsEmpty())
		PreSavePath = FPlatformProcess::UserDir();

	if (DesktopPlatform != NULL)
	{
		TArray<FString> SaveFilenames;
		const FString FileTypes = TEXT("Texture (*.png)|*.png");
		bSave = DesktopPlatform->SaveFileDialog
		(
			NULL,
			TEXT("Save"),
			PreSavePath,//FPaths::ProjectDir(),
			FileName,
			FileTypes,
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bSave)
		{
			path = SaveFilenames[0];
			FString FileName, FileExt;
			FPaths::Split(path, PreSavePath, FileName, FileExt);
		}
	}
}

UTexture2D* UPicItemWidget::LoadTexture(FString path)
{
	//return FImageUtils::ImportFileAsTexture2D(path);
	/*
	if (path.EndsWith(TEXT(".TGA")) || path.EndsWith(TEXT(".png")) || path.EndsWith(TEXT(".bmp")) || path.EndsWith(TEXT(".jpg")) || path.EndsWith(TEXT(".jpeg")))
	{
		FString OpenPath, FileName, FileExt;
		FPaths::Split(path, OpenPath, FileName, FileExt);

		UFactory* Factory = Cast<UFactory>(UTexture2D::StaticClass()->GetDefaultObject());
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

		TArray<FString> Paths;
		Paths.Add(path);
		TArray<UObject*> ReturnObjects = AssetToolsModule.Get().ImportAssets(Paths, OpenPath, Factory);

		UTexture2D* NewTex = nullptr;
		if(ReturnObjects.Num() > 0)
			NewTex = Cast<UTexture2D>(ReturnObjects[0]);

		if (NewTex)
		{
			NewTex->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
			NewTex->SRGB = false;

			return NewTex;
		}
	}

	return nullptr;
	*/
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *path))
		return nullptr;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper;
	if (path.EndsWith(TEXT(".png")))
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	}
	else if(path.EndsWith(TEXT(".bmp")))
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
	}
	else if (path.EndsWith(TEXT(".jpg")) || path.EndsWith(TEXT(".jpeg")))
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	}

	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		int32 Width = 0;
		int32 Height = 0;
		TArray<uint8> RawData;

		Width = ImageWrapper->GetWidth();
		Height = ImageWrapper->GetHeight();
		
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
		{
			UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
			if (NewTexture)
			{
				uint8* MipData = static_cast<uint8*>(NewTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
				FMemory::Memcpy(MipData, RawData.GetData(), NewTexture->PlatformData->Mips[0].BulkData.GetBulkDataSize());
				NewTexture->PlatformData->Mips[0].BulkData.Unlock();
				NewTexture->UpdateResource();
				NewTexture->SRGB = true;
				return NewTexture;
			}
		}
	}
	
	return nullptr;
}

void UPicItemWidget::ExportRenderTarget2D(UTextureRenderTarget2D* TexRT, const FString& FileName)
{
	if (TexRT)
	{
		UObject* NewObj = nullptr;
		NewObj = TexRT->ConstructTexture2D(this, FileName, TexRT->GetMaskedFlags() | RF_Public | RF_Standalone, CTF_Default | CTF_AllowMips, NULL);
		UTexture2D* NewTex = Cast<UTexture2D>(NewObj);

		if (NewTex != nullptr)
		{
			NewTex->CompressionSettings = TextureCompressionSettings::TC_HDR;
			NewTex->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

			TArray<UObject*> ObjectsToExport;
			ObjectsToExport.Add(NewObj);
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
			AssetToolsModule.Get().ExportAssetsWithDialog(ObjectsToExport, true);
		}
	}
}

bool UPicItemWidget::ExportTexture2D(UTexture2D* Tex2D, const FString& FilePath, const FString& FileName)
{
	bool bSuccess = false;
	FString TotalFileName = FPaths::Combine(*FilePath, *FileName);
	FText PathError;
	FPaths::ValidatePath(TotalFileName, &PathError);

	if (!PathError.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: Path wrong ____ %s"), *(PathError.ToString()));
	}
	else if (FileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: filename empty"));
	}
	else
	{
		if (Tex2D)
		{
			bSuccess = true;

			EPixelFormat Format = Tex2D->GetPixelFormat();
			int32 ImageBytes = CalculateImageBytes(Tex2D->GetSizeX(), Tex2D->GetSizeY(), 0, Format);

			uint8* MipData = static_cast<uint8*>(Tex2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

			IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
			TSharedPtr<IImageWrapper> PNGImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			PNGImageWrapper->SetRaw(MipData, ImageBytes * sizeof(uint8), Tex2D->GetSizeX(), Tex2D->GetSizeY(), ERGBFormat::BGRA, 8);

			const TArray64<uint8>& PNGData = PNGImageWrapper->GetCompressed(100);
			FFileHelper::SaveArrayToFile(PNGData, *TotalFileName);

			Tex2D->PlatformData->Mips[0].BulkData.Unlock();
		}
	}

	return bSuccess;
}

