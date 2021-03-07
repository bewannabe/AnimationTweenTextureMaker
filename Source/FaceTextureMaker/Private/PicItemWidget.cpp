// Fill out your copyright notice in the Description page of Project Settings.

#include "PicItemWidget.h"

#include "Engine/TextureDefines.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Paths.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "FileHelper.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "RendererUtils.h"
#include "Exporters/Exporter.h"
#include "AssetExportTask.h"
#include "BufferArchive.h"


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
		const FString FileTypes = TEXT("Texture (*.TGA)|*.TGA|Texture (*.png)|*.png|All Files (*.TGA;*.png)|*.TGA;*.png");
		bSave = DesktopPlatform->SaveFileDialog
		(
			NULL,
			TEXT("Save"),
			PreSavePath,
			FileName,
			FileTypes,
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bSave)
		{
			path = SaveFilenames[0];
			FString curFileName, FileExt;
			FPaths::Split(path, PreSavePath, curFileName, FileExt);
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

bool UPicItemWidget::ExportRenderTarget2D(UTextureRenderTarget2D* TexRT, const FString& FileName)
{
	if (TexRT)
	{
		UObject* NewObj = nullptr;
		EObjectFlags InObjectFlags = TexRT->GetMaskedFlags() | RF_Public | RF_Standalone;

		UTexture2D* Result = NULL;
		// Check render target size is valid and power of two.
		const bool bIsValidSize = (TexRT->SizeX != 0 && !(TexRT->SizeX & (TexRT->SizeX - 1)) &&
			TexRT->SizeY != 0 && !(TexRT->SizeY & (TexRT->SizeY - 1)));
		// The r2t resource will be needed to read its surface contents

		const EPixelFormat PixelFormat = TexRT->GetFormat();

		FRenderTarget* RenderTarget = TexRT->GameThread_GetRenderTargetResource();
		// exit if source is not compatible.
		if (bIsValidSize == false || RenderTarget == NULL)
			return false;

		TArray<FColor> SurfData;
		RenderTarget->ReadPixels(SurfData);

		// create the 2d texture
		Result = UTexture2D::CreateTransient(TexRT->SizeX, TexRT->SizeY, TexRT->GetFormat(), FName(*FileName));

		if (Result == NULL)
			return false;

		uint8* MipData = static_cast<uint8*>(Result->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
		FMemory::Memcpy(MipData, SurfData.GetData(), Result->PlatformData->Mips[0].BulkData.GetBulkDataSize());
		Result->PlatformData->Mips[0].BulkData.Unlock();
		Result->UpdateResource();
		Result->SRGB = TexRT->IsSRGB();
		return ExportTexture2DWithDialog(Result);
	}

	return false;
}

bool UPicItemWidget::ExportTexture2D(UTexture2D* Tex2D)
{
	return ExportTexture2DWithDialog(Tex2D);
}

bool UPicItemWidget::ExportTexture2DWithDialog(UTexture2D* ObjectToExport)
{
	FString Path;
	SaveDialog(Path, ObjectToExport->GetName());

	if (Path.EndsWith(".TGA"))
		return ExportToTGA(ObjectToExport, Path);
	else if (Path.EndsWith(".png"))
		return ExportToPNG(ObjectToExport, Path);

	return false;
}

bool UPicItemWidget::ExportToTGA(UTexture2D* ObjectToExport, const FString& FileName)
{
	FText PathError;
	FPaths::ValidatePath(FileName, &PathError);

	if (!PathError.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: Path wrong ____ %s"), *(PathError.ToString()));
		return false;
	}
	else if (FileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: filename empty"));
		return false;
	}


	FBufferArchive Ar;
	UTexture2D* Texture = ObjectToExport;

	if (Texture == nullptr || (Texture->GetPixelFormat() != TSF_BGRA8 && Texture->GetPixelFormat() != TSF_RGBA16))
	{
		return false;
	}

	const bool bIsRGBA16 = Texture->GetPixelFormat() == TSF_RGBA16;

	const int32 BytesPerPixel = bIsRGBA16 ? 8 : 4;

	int32 SizeX = Texture->GetSizeX();
	int32 SizeY = Texture->GetSizeY();
	uint8* RawData = static_cast<uint8*>(Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	// If we should export the file with no alpha info.  
	// If the texture is compressed with no alpha we should definitely not export an alpha channel
	bool bExportWithAlpha = true;
	if (bExportWithAlpha)
	{
		// If the texture isn't compressed with no alpha scan the texture to see if the alpha values are all 255 which means we can skip exporting it.
		// This is a relatively slow process but we are just exporting textures 
		bExportWithAlpha = false;
		const int32 AlphaOffset = bIsRGBA16 ? 7 : 3;
		for (int32 Y = SizeY - 1; Y >= 0; --Y)
		{
			uint8* Color = &RawData[Y * SizeX * BytesPerPixel];
			for (int32 X = SizeX; X > 0; --X)
			{
				// Skip color info
				Color += AlphaOffset;
				// Get Alpha value then increment the pointer past it for the next pixel
				uint8 Alpha = *Color++;
				if (Alpha != 255)
				{
					// When a texture is imported with no alpha, the alpha bits are set to 255
					// So if the texture has non 255 alpha values, the texture is a valid alpha channel
					bExportWithAlpha = true;
					break;
				}
			}
			if (bExportWithAlpha)
			{
				break;
			}
		}
	}

	const int32 OriginalWidth = SizeX;
	const int32 OriginalHeight = SizeY;

	FTGAFileHeader TGA;
	FMemory::Memzero(&TGA, sizeof(TGA));
	TGA.ImageTypeCode = 2;
	TGA.BitsPerPixel = bExportWithAlpha ? 32 : 24;
	TGA.Height = OriginalHeight;
	TGA.Width = OriginalWidth;
	Ar.Serialize(&TGA, sizeof(TGA));

	if (bExportWithAlpha && !bIsRGBA16)
	{
		for (int32 Y = 0; Y < OriginalHeight; Y++)
		{
			// If we aren't skipping alpha channels we can serialize each line
			Ar.Serialize(&RawData[(OriginalHeight - Y - 1) * OriginalWidth * 4], OriginalWidth * 4);
		}
	}
	else
	{
		// Serialize each pixel
		for (int32 Y = OriginalHeight - 1; Y >= 0; --Y)
		{
			uint8* Color = &RawData[Y * OriginalWidth * BytesPerPixel];
			for (int32 X = OriginalWidth; X > 0; --X)
			{
				if (bIsRGBA16)
				{
					Ar << Color[1];
					Ar << Color[3];
					Ar << Color[5];
					if (bExportWithAlpha)
					{
						Ar << Color[7];
					}
					Color += 8;
				}
				else
				{
					Ar << *Color++;
					Ar << *Color++;
					Ar << *Color++;
					// Skip alpha channel since we are exporting with no alpha
					Color++;
				}
			}
		}
	}

	Texture->PlatformData->Mips[0].BulkData.Unlock();

	FTGAFileFooter Ftr;
	FMemory::Memzero(&Ftr, sizeof(Ftr));
	FMemory::Memcpy(Ftr.Signature, "TRUEVISION-XFILE", 16);
	Ftr.TrailingPeriod = '.';
	Ar.Serialize(&Ftr, sizeof(Ftr));

	return FFileHelper::SaveArrayToFile(Ar, *FileName);
}

bool UPicItemWidget::ExportToPNG(UTexture2D* ObjectToExport, const FString& FileName)
{
	FText PathError;
	FPaths::ValidatePath(FileName, &PathError);

	if (!PathError.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: Path wrong ____ %s"), *(PathError.ToString()));
		return false;
	}
	else if (FileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: filename empty"));
		return false;
	}

	if (ObjectToExport == nullptr)
		return false;

	EPixelFormat Format = ObjectToExport->GetPixelFormat();
	int32 ImageBytes = CalculateImageBytes(ObjectToExport->GetSizeX(), ObjectToExport->GetSizeY(), 0, Format);

	uint8* MipData = static_cast<uint8*>(ObjectToExport->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> PNGImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	PNGImageWrapper->SetRaw(MipData, ImageBytes * sizeof(uint8), ObjectToExport->GetSizeX(), ObjectToExport->GetSizeY(), ERGBFormat::BGRA, 8);

	ObjectToExport->PlatformData->Mips[0].BulkData.Unlock();

	const TArray64<uint8>& PNGData = PNGImageWrapper->GetCompressed(100);
	return FFileHelper::SaveArrayToFile(PNGData, *FileName);
}

