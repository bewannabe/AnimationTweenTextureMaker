// Fill out your copyright notice in the Description page of Project Settings.


#include "Texture8ssedtMaker.h"


UTexture2D* UTexture8ssedtMaker::Generate8ssedtTexture(UTexture2D* source)
{
	if (source == nullptr)
		return nullptr;

	uint8* MipData = static_cast<uint8*>(source->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 y = 0; y < HEIGHT; y++)
	{
		for (int32 x = 0; x < WIDTH; x++)
		{
			uint8 r, g, b, a;

			const int32 X = source->GetSizeX() * ((float)x / (float)WIDTH);
			const int32 Y = source->GetSizeY() * ((float)y / (float)HEIGHT);

			const int32 Index = ((Y * source->GetSizeX()) + X) * 4;

			r = MipData[Index + 0];
			g = MipData[Index + 1];
			b = MipData[Index + 2];
			a = MipData[Index + 3];

			if ((int32)g > 128)
			{
				Put(grid1, x, y, FPoint());
				Put(grid2, x, y, FPoint(9999,9999));
			}
			else {
				Put(grid2, x, y, FPoint());
				Put(grid1, x, y, FPoint(9999, 9999));
			}
		}
	}

	source->PlatformData->Mips[0].BulkData.Unlock();

	GenerateSDF(grid1);
	GenerateSDF(grid2);

	TArray<uint8> dest = TArray<uint8>();
	dest.Reserve(WIDTH*HEIGHT);
	for (int32 y = 0; y < HEIGHT; y++)
	{
		for (int32 x = 0; x < WIDTH; x++)
		{
			// Calculate the actual distance from the dx/dy
			int32 dist1 = (int32)(sqrt((double)Get(grid1, x, y).DistSq()));
			int32 dist2 = (int32)(sqrt((double)Get(grid2, x, y).DistSq()));
			int32 dist = dist1 - dist2;

			// Clamp and scale it, just for display purposes.
			int32 c = dist * 3 + 128;
			if (c < 0) c = 0;
			if (c > 255) c = 255;
			c = 255 - c;

			dest.Add((uint8)c);
			dest.Add((uint8)c);
			dest.Add((uint8)c);
			dest.Add((uint8)c);
		}
	}

	Texture8ssedt = UTexture2D::CreateTransient(WIDTH, HEIGHT, PF_B8G8R8A8);
	if (Texture8ssedt)
	{
		uint8* MipData = static_cast<uint8*>(Texture8ssedt->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
		FMemory::Memcpy(MipData, dest.GetData(), Texture8ssedt->PlatformData->Mips[0].BulkData.GetBulkDataSize());
		Texture8ssedt->PlatformData->Mips[0].BulkData.Unlock();
		Texture8ssedt->UpdateResource();
		Texture8ssedt->SRGB = false;
	}

	return Texture8ssedt;
}

UTexture2D* UTexture8ssedtMaker::GetTexture()
{
	return Texture8ssedt;
}

FPoint UTexture8ssedtMaker::Get(FGrid &g, int32 x, int32 y)
{
	if (x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT)
		return g.grid[y][x];
	else
		return FPoint(9999, 9999);
}

void UTexture8ssedtMaker::Put(FGrid &g, int32 x, int32 y, const FPoint &p)
{
	g.grid[y][x] = p;
}

void UTexture8ssedtMaker::Compare(FGrid &g, FPoint &p, int32 x, int32 y, int32 offsetx, int32 offsety)
{
	FPoint other = Get(g, x + offsetx, y + offsety);
	other.dx += offsetx;
	other.dy += offsety;

	if (other.DistSq() < p.DistSq())
		p = other;
}

void UTexture8ssedtMaker::GenerateSDF(FGrid &g)
{
	// Pass 0
	for (int32 y = 0; y < HEIGHT; y++)
	{
		for (int32 x = 0; x < WIDTH; x++)
		{
			FPoint p = Get(g, x, y);
			Compare(g, p, x, y, -1, 0);
			Compare(g, p, x, y, 0, -1);
			Compare(g, p, x, y, -1, -1);
			Compare(g, p, x, y, 1, -1);
			Put(g, x, y, p);
		}

		for (int32 x = WIDTH - 1; x >= 0; x--)
		{
			FPoint p = Get(g, x, y);
			Compare(g, p, x, y, 1, 0);
			Put(g, x, y, p);
		}
	}

	// Pass 1
	for (int32 y = HEIGHT - 1; y >= 0; y--)
	{
		for (int32 x = WIDTH - 1; x >= 0; x--)
		{
			FPoint p = Get(g, x, y);
			Compare(g, p, x, y, 1, 0);
			Compare(g, p, x, y, 0, 1);
			Compare(g, p, x, y, -1, 1);
			Compare(g, p, x, y, 1, 1);
			Put(g, x, y, p);
		}

		for (int32 x = 0; x < WIDTH; x++)
		{
			FPoint p = Get(g, x, y);
			Compare(g, p, x, y, -1, 0);
			Put(g, x, y, p);
		}
	}
}
