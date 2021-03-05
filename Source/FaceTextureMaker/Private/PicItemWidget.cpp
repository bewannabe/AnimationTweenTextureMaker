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

void UPicItemWidget::ExportRenderTarget2D(UTextureRenderTarget2D* TexRT, const FString& FileName)
{
	if (TexRT)
	{
		UObject* NewObj = nullptr;
		NewObj = TexRT->ConstructTexture2D(this, FileName, TexRT->GetMaskedFlags() | RF_Public | RF_Standalone, CTF_Default | CTF_AllowMips, NULL);
		UTexture2D* NewTex = Cast<UTexture2D>(NewObj);

		if (NewTex != nullptr)
		{
			ExportAssetWithDialog(NewObj);
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

bool UPicItemWidget::ExportAssetWithDialog(UObject* ObjectToExport)
{
	FString LastExportPath = !PreExportPath.IsEmpty() ? PreExportPath : FPlatformProcess::UserDir();

	TArray<UExporter*> Exporters;
	auto TransientPackage = GetTransientPackage();
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UExporter::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
		{
			UExporter* Exporter = NewObject<UExporter>(TransientPackage, *It);
			Exporters.Add(Exporter);
		}
	}

	TArray<FString> AllFileTypes;
	TArray<FString> AllExtensions;
	TArray<FString> PreferredExtensions;

	for (int32 ExporterIndex = Exporters.Num() - 1; ExporterIndex >= 0; --ExporterIndex)
	{
		UExporter* Exporter = Exporters[ExporterIndex];
		if (Exporter->SupportedClass)
		{
			const bool bObjectIsSupported = Exporter->SupportsObject(ObjectToExport);
			if (bObjectIsSupported)
			{
				// Get a string representing of the exportable types.
				check(Exporter->FormatExtension.Num() == Exporter->FormatDescription.Num());
				check(Exporter->FormatExtension.IsValidIndex(Exporter->PreferredFormatIndex));
				for (int32 FormatIndex = Exporter->FormatExtension.Num() - 1; FormatIndex >= 0; --FormatIndex)
				{
					const FString& FormatExtension = Exporter->FormatExtension[FormatIndex];
					const FString& FormatDescription = Exporter->FormatDescription[FormatIndex];

					if (FormatIndex == Exporter->PreferredFormatIndex)
					{
						PreferredExtensions.Add(FormatExtension);
					}
					AllFileTypes.Add(FString::Printf(TEXT("%s (*.%s)|*.%s"), *FormatDescription, *FormatExtension, *FormatExtension));
					AllExtensions.Add(FString::Printf(TEXT("*.%s"), *FormatExtension));
				}
			}
		}
	}
	
	if (PreferredExtensions.Num() == 0)
	{
		return false;
	}

	
	// If FBX is listed, make that the most preferred option
	const FString PreferredExtension = TEXT("FBX");
	int32 ExtIndex = PreferredExtensions.Find(PreferredExtension);
	if (ExtIndex > 0)
	{
		PreferredExtensions.RemoveAt(ExtIndex);
		PreferredExtensions.Insert(PreferredExtension, 0);
	}
	FString FirstExtension = PreferredExtensions[0];

	// If FBX is listed, make that the first option here too, then compile them all into one string
	check(AllFileTypes.Num() == AllExtensions.Num())
		for (ExtIndex = 1; ExtIndex < AllFileTypes.Num(); ++ExtIndex)
		{
			const FString FileType = AllFileTypes[ExtIndex];
			if (FileType.Contains(PreferredExtension))
			{
				AllFileTypes.RemoveAt(ExtIndex);
				AllFileTypes.Insert(FileType, 0);

				const FString Extension = AllExtensions[ExtIndex];
				AllExtensions.RemoveAt(ExtIndex);
				AllExtensions.Insert(Extension, 0);
			}
		}
	FString FileTypes;
	FString Extensions;
	for (ExtIndex = 0; ExtIndex < AllFileTypes.Num(); ++ExtIndex)
	{
		if (FileTypes.Len())
		{
			FileTypes += TEXT("|");
		}
		FileTypes += AllFileTypes[ExtIndex];

		if (Extensions.Len())
		{
			Extensions += TEXT(";");
		}
		Extensions += AllExtensions[ExtIndex];
	}
	FileTypes = FString::Printf(TEXT("%s|All Files (%s)|%s"), *FileTypes, *Extensions, *Extensions);

	FString SaveFileName;

	TArray<FString> SaveFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bSave = false;
	if (DesktopPlatform)
	{
		bSave = DesktopPlatform->SaveFileDialog(
			NULL,
			FText::Format(NSLOCTEXT("UnrealEd", "Save_F", "Save: {0}"), FText::FromString(ObjectToExport->GetName())).ToString(),
			*LastExportPath,
			*ObjectToExport->GetName(),
			*FileTypes,
			EFileDialogFlags::None,
			SaveFilenames
		);
	}

	if (!bSave)
	{
		return false;
	}
	SaveFileName = FString(SaveFilenames[0]);
	FString tempFileName, tempFileExt;
	FPaths::Split(SaveFileName, PreExportPath, tempFileName, tempFileExt);

	
	// Copy off the selected path for future export operations.
	LastExportPath = SaveFileName;

	const FString ObjectExportPath(FPaths::GetPath(SaveFileName));
	const bool bFileInSubdirectory = ObjectExportPath.Contains(TEXT("/"));
	if (bFileInSubdirectory && (!IFileManager::Get().MakeDirectory(*ObjectExportPath, true)))
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: Path wrong ____ %s"), *ObjectExportPath);
	}
	else if (IFileManager::Get().IsReadOnly(*SaveFileName))
	{
		UE_LOG(LogTemp, Warning, TEXT("PicItem: Couldn't write to file %s . Maybe file is read-only? "), *SaveFileName);
	}
	else
	{
		// We have a writeable file.  Now go through that list of exporters again and find the right exporter and use it.
		TArray<UExporter*>	ValidExporters;

		for (int32 ExporterIndex = 0; ExporterIndex < Exporters.Num(); ++ExporterIndex)
		{
			UExporter* Exporter = Exporters[ExporterIndex];
			if (Exporter->SupportsObject(ObjectToExport))
			{
				check(Exporter->FormatExtension.Num() == Exporter->FormatDescription.Num());
				for (int32 FormatIndex = 0; FormatIndex < Exporter->FormatExtension.Num(); ++FormatIndex)
				{
					const FString& FormatExtension = Exporter->FormatExtension[FormatIndex];
					if (FCString::Stricmp(*FormatExtension, *FPaths::GetExtension(SaveFileName)) == 0 ||
						FCString::Stricmp(*FormatExtension, TEXT("*")) == 0)
					{
						ValidExporters.Add(Exporter);
						break;
					}
				}
			}
		}

		// Handle the potential of multiple exporters being found
		UExporter* ExporterToUse = NULL;
		if (ValidExporters.Num() == 1)
		{
			ExporterToUse = ValidExporters[0];
		}
		else if (ValidExporters.Num() > 1)
		{
			// Set up the first one as default
			ExporterToUse = ValidExporters[0];

			// ...but search for a better match if available
			for (int32 ExporterIdx = 0; ExporterIdx < ValidExporters.Num(); ExporterIdx++)
			{
				if (ValidExporters[ExporterIdx]->GetClass()->GetFName() == ObjectToExport->GetExporterName())
				{
					ExporterToUse = ValidExporters[ExporterIdx];
					break;
				}
			}
		}

		// If an exporter was found, use it.
		if (ExporterToUse)
		{
			ExporterToUse->SetBatchMode(false);
			ExporterToUse->SetCancelBatch(false);
			ExporterToUse->SetShowExportOption(true);
			ExporterToUse->AddToRoot();

			UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
			ExportTask->Object = ObjectToExport;
			ExportTask->Exporter = ExporterToUse;
			ExportTask->Filename = SaveFileName;
			ExportTask->bSelected = false;
			ExportTask->bReplaceIdentical = true;
			ExportTask->bPrompt = false;
			ExportTask->bUseFileArchive = ObjectToExport->IsA(UPackage::StaticClass());
			ExportTask->bWriteEmptyFiles = false;

			UExporter::RunAssetExportTask(ExportTask);

			if (ExporterToUse->GetBatchMode() && ExporterToUse->GetCancelBatch())
			{
				//Exit the export file loop when there is a cancel all
				return false;
			}

			ExporterToUse->SetBatchMode(false);
			ExporterToUse->SetCancelBatch(false);
			ExporterToUse->SetShowExportOption(true);
			ExporterToUse->RemoveFromRoot();
		}
	}
	
	return true;
}

