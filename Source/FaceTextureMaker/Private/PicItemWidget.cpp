// Fill out your copyright notice in the Description page of Project Settings.

#include "PicItemWidget.h"

#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
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
	if (DesktopPlatform != NULL)
	{
		const FString FileTypes = TEXT("Texture (*.png;*.bmp;*.jpg;*.jpeg)|*.png;*.bmp;*.jpg;*.jpeg");
		bOpened = DesktopPlatform->OpenFileDialog
		(
			NULL,
			TEXT("Switch Textures"),
			FPlatformProcess::UserDir(),
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
}

void UPicItemWidget::SaveDialog(FString& path)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	bool bSave = false;
	if (DesktopPlatform != NULL)
	{
		TArray<FString> SaveFilenames;
		const FString FileTypes = TEXT("Texture (*.png)|*.png");
		bSave = DesktopPlatform->SaveFileDialog
		(
			NULL,
			TEXT("Save"),
			FPlatformProcess::UserDir(),//FPaths::ProjectDir(),
			TEXT("Face"),
			FileTypes,
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bSave)
		{
			path = SaveFilenames[0];
		}
	}
}

UTexture2D* UPicItemWidget::LoadTexture(FString path)
{
	//return FImageUtils::ImportFileAsTexture2D(path);
	
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

bool UPicItemWidget::ExportRenderTarget2DToPNG(UTextureRenderTarget2D* TexRT, const FString& FilePath, const FString& FileName)
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
		EPixelFormat Format = TexRT->GetFormat();
		int32 ImageBytes = CalculateImageBytes(TexRT->SizeX, TexRT->SizeY, 0, Format);
		TArray64<uint8> RawData;
		RawData.AddUninitialized(ImageBytes);

		FRenderTarget* RenderTarget = TexRT->GameThread_GetRenderTargetResource();
		bSuccess = RenderTarget->ReadPixelsPtr((FColor*)RawData.GetData());

		if (bSuccess)
		{
			IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
			TSharedPtr<IImageWrapper> PNGImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			PNGImageWrapper->SetRaw(RawData.GetData(), RawData.GetAllocatedSize(), TexRT->SizeX, TexRT->SizeX, ERGBFormat::BGRA, 8);

			const TArray64<uint8>& PNGData = PNGImageWrapper->GetCompressed(100);
			FFileHelper::SaveArrayToFile(PNGData, *TotalFileName);
		}
	}

	return bSuccess;
}

