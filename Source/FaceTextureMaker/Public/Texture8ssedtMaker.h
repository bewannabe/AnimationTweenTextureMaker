// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Texture8ssedtMaker.generated.h"

#define WIDTH  1024
#define HEIGHT 1024

USTRUCT()
struct FPoint
{
	GENERATED_BODY()

public:
	int32 dx, dy;
	
	FPoint()
		:dx(0), dy(0)
	{}

	FPoint(int32 _dx, int32 _dy)
		:dx(_dx), dy(_dy)
	{}

	FPoint(const FPoint& copy)
	{
		dx = copy.dx;
		dy = copy.dy;
	}

	FPoint& operator=(const FPoint& copy)
	{
		dx = copy.dx;
		dy = copy.dy;

		return *this;
	}

	int32 DistSq() const { return dx * dx + dy * dy; }
};

USTRUCT()
struct FGrid
{
	GENERATED_BODY()

public:
	FPoint grid[HEIGHT][WIDTH];
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class FACETEXTUREMAKER_API UTexture8ssedtMaker : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Texture8ssedtMaker")
		UTexture2D* Generate8ssedtTexture(UTexture2D* source);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Texture8ssedtMaker")
		UTexture2D* GetTexture();

private:
	FPoint Get(FGrid &g, int32 x, int32 y);
	void Put(FGrid &g, int32 x, int32 y, const FPoint &p);
	void Compare(FGrid &g, FPoint &p, int32 x, int32 y, int32 offsetx, int32 offsety);
	void GenerateSDF(FGrid &g);

	FGrid grid1;
	FGrid grid2;

	UTexture2D* Texture8ssedt;
};
