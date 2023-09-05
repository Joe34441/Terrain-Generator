// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/Engine.h"
#include "Generator.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

USTRUCT()
struct FTerrainLayer
{
	GENERATED_BODY()

	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector2D> UV0;
	TArray<FVector> Normals;
	TArray<struct FProcMeshTangent> Tangents;
	TArray<FVector> CubePositions;
	int CubeCount;

	TArray<FVector> UniversalNormals;
	TArray<struct FProcMeshTangent> UniversalTangents;
};

UCLASS()
class TERRAIN_GENERATOR_API AGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGenerator();

	UPROPERTY(EditAnywhere)
	bool toggle = false;

	UPROPERTY(EditAnywhere)
	bool clear = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1", ClampMax = "18"))
	int Chunks = 6;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1", ClampMax = "10000"))
	int GenerationSeed = 512;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "100", DisplayName = "Mountain Density (%)"))
	int MountainDensity = 20;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1", ClampMax = "100"))
	float MountainScale = 3;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float WaterHeight = -0.1;

	UPROPERTY(EditAnywhere)
	bool DrawUnderneath = false;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* EarthMaterial;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* WaterMaterial;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UProceduralMeshComponent* ProceduralMesh;

	FTerrainLayer WaterLayer;
	FTerrainLayer GrassLayer;

	TArray<FVector> borderCubePositions;

	TSet<FVector> currentLayerCubePositionsSet;

	int meshSectionsCount;
	int seedX;
	int seedY;

	int worldScale = 100;

	float percentileNoise;
	float percentileTopNoise;

	float baseNoiseScale = 0.008f;
	float baseNoiseMultiplier = 0.2f;

	bool useDetailedLighting = false;


	void PreGeneration(int chunkSize, int chunks, float noiseScale);
	void GenerateTerrain();
	void GenerateChunk(int chunkSize, int chunkX, int chunkY);
	void GenerateTerrainFromNoise(float noiseScale, int chunkSize, int chunkX, int chunkY);
	void FindOuterBorderCubes(int chunkSize, int chunkX, int chunkY, float noiseScale);
	void FillChunkGaps();
	void AddCubes();
	void AddCube(FTerrainLayer& layer, float scale, float x, float y, float z);
	void CreateCubeTriangles(FTerrainLayer& layer, int cubeIndex, FVector cubePos);
	void CreateTriangle(FTerrainLayer& layer, int x, int y, int z);
	void TryCreateFace(FTerrainLayer& layer, FVector cubePos, FVector neighbourOffset, FVector offset1, FVector offset2, int indexOffet);
	void CalculateLightingData();
	void CalculateLayerLightingData(FTerrainLayer& layer);
	void ResetLayers(bool clearMesh);
	void ResetLayer(FTerrainLayer& layer);
	void CreateLayerCubesTriangles(FTerrainLayer& layer);
	void DrawTerrain();
	void DrawTerrainSection(int meshSectionIndex, FTerrainLayer& layer, UMaterialInterface* material);

	float CalculateHeightFromNoise(int xPos, int yPos, float noiseScale);
	float CubicInterpolation(float value);
};
