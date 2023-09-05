// Fill out your copyright notice in the Description page of Project Settings.


#include "Generator.h"

// Sets default values
AGenerator::AGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//setup mesh component
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	ProceduralMesh->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!toggle) return;
	toggle = false;

	ResetLayers(true);

	if (clear)
	{
		clear = false;
		return;
	}

	double startTotal = FPlatformTime::Seconds();
	GenerateTerrain();
	double endTotal = FPlatformTime::Seconds();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Total time taken: %f seconds"), endTotal - startTotal));
}

// Called every frame
void AGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGenerator::GenerateTerrain()
{
	double start = FPlatformTime::Seconds();
	PreGeneration(32, Chunks, 0.008);
	double end = FPlatformTime::Seconds();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Time taken pre-gen: %f seconds"), end - start));


	for (int chunkX = 0; chunkX < Chunks; ++chunkX)
	{
		for (int chunkY = 0; chunkY < Chunks; ++chunkY)
		{
			GenerateChunk(32, chunkX, chunkY);
			ResetLayers(false);
		}
	}
}

void AGenerator::GenerateChunk(int chunkSize, int chunkX, int chunkY)
{
	//calculate point heights for this chunk
	GenerateTerrainFromNoise(baseNoiseScale, chunkSize, chunkX, chunkY);
	//create the cubes
	AddCubes();
	//perform correct lighting calculations
	CalculateLightingData();
	//draw the terrain chunk
	DrawTerrain();
}

void AGenerator::AddCubes()
{
	//iterate over every cube position on each layer and create the cubes
	for (FVector pos : WaterLayer.CubePositions)
	{
		AddCube(WaterLayer, worldScale, pos.X, pos.Y, pos.Z);
	}
	for (FVector pos : GrassLayer.CubePositions)
	{
		AddCube(GrassLayer, worldScale, pos.X, pos.Y, pos.Z);
	}

	//create triangle data for each layer's cubes
	CreateLayerCubesTriangles(WaterLayer);
	CreateLayerCubesTriangles(GrassLayer);
}

void AGenerator::AddCube(FTerrainLayer& layer, float scale, float x, float y, float z)
{
	//only calculate scaled values once
	float xScaled = x * scale;
	float yScaled = y * scale;
	float zScaled = z * scale;

	//top verticies
	layer.Vertices.Add(FVector(xScaled, yScaled, zScaled + scale));
	layer.Vertices.Add(FVector(xScaled, yScaled + scale, zScaled + scale));
	layer.Vertices.Add(FVector(xScaled + scale, yScaled, zScaled + scale));
	layer.Vertices.Add(FVector(xScaled + scale, yScaled + scale, zScaled + scale));
	//bottom vertices
	layer.Vertices.Add(FVector(xScaled, yScaled, zScaled));
	layer.Vertices.Add(FVector(xScaled + scale, yScaled, zScaled));
	layer.Vertices.Add(FVector(xScaled, yScaled + scale, zScaled));
	layer.Vertices.Add(FVector(xScaled + scale, yScaled + scale, zScaled));

	//increment the cube count
	layer.CubeCount++;
}

void AGenerator::CreateCubeTriangles(FTerrainLayer& layer, int cubeIndex, FVector cubePos)
{
	//get offset so triangles will use correct set of vertices
	int offset = cubeIndex * 8;

	//create cube faces
	TryCreateFace(layer, cubePos, FVector(0, 0, 1), FVector(0, 1, 2), FVector(1, 3, 2), offset); //face 1
	TryCreateFace(layer, cubePos, FVector(0, -1, 0), FVector(4, 0, 5), FVector(0, 2, 5), offset); //face 2
	TryCreateFace(layer, cubePos, FVector(-1, 0, 0), FVector(6, 1, 4), FVector(1, 0, 4), offset); //face 3
	TryCreateFace(layer, cubePos, FVector(0, 1, 0), FVector(7, 3, 6), FVector(3, 1, 6), offset); //face 4
	TryCreateFace(layer, cubePos, FVector(1, 0, 0), FVector(5, 2, 7), FVector(2, 3, 7), offset); //face 5

	if (!DrawUnderneath) return;

	TryCreateFace(layer, cubePos, FVector(0, 0, -1), FVector(5, 6, 4), FVector(6, 5, 7), offset); //face 6
}

void AGenerator::TryCreateFace(FTerrainLayer& layer, FVector cubePos, FVector neighbourOffset, FVector offset1, FVector offset2, int indexOffet)
{
	//faces are against eachother
	if (currentLayerCubePositionsSet.Contains(cubePos + neighbourOffset)) return;
	//faces are not visible from top view
	if (neighbourOffset.Z == 0)
	{
		FVector checkPos = cubePos + neighbourOffset;
		for (int i = 1; i < 10; ++i) //only check for up to 10 positions higher
		{
			if (currentLayerCubePositionsSet.Contains(FVector(checkPos.X, checkPos.Y, checkPos.Z + i))) return;
		}
	}

	//create face
	CreateTriangle(layer, offset1.X + indexOffet, offset1.Y + indexOffet, offset1.Z + indexOffet); //triangle 1
	CreateTriangle(layer, offset2.X + indexOffet, offset2.Y + indexOffet, offset2.Z + indexOffet); //triangle 2
}

void AGenerator::CreateTriangle(FTerrainLayer& layer, int point1, int point2, int point3)
{
	//add points to draw triangle with
	layer.Triangles.Add(point1);
	layer.Triangles.Add(point2);
	layer.Triangles.Add(point3);
}

void AGenerator::ResetLayers(bool clearMesh)
{
	if (clearMesh)
	{
		//reset entire mesh data
		ProceduralMesh->ClearAllMeshSections();
		meshSectionsCount = 0;
		percentileNoise = 0;
		percentileTopNoise = 0;
	}
	
	//reset layer data
	ResetLayer(WaterLayer);
	ResetLayer(GrassLayer);

	borderCubePositions.Reset();
}

void AGenerator::ResetLayer(FTerrainLayer& layer)
{
	//reset each piece of data in this layer
	layer.Vertices.Reset();
	layer.Triangles.Reset();
	layer.CubePositions.Reset();
	layer.Normals.Reset();
	layer.Tangents.Reset();
	layer.UniversalNormals.Reset();
	layer.UniversalTangents.Reset();
	layer.CubeCount = 0;
}

void AGenerator::CreateLayerCubesTriangles(FTerrainLayer& layer)
{
	//update the set for faster access when creating triangles
	currentLayerCubePositionsSet.Reset();
	currentLayerCubePositionsSet.Append(layer.CubePositions);

	int i = 0;
	for (FVector pos : layer.CubePositions)
	{
		CreateCubeTriangles(layer, i, pos);
		i++;
	}

	currentLayerCubePositionsSet.Reset();
}

void AGenerator::GenerateTerrainFromNoise(float noiseScale, int chunkSize, int chunkX, int chunkY)
{
	//perform x and y multiplication once
	int xPosStart = chunkX * chunkSize;
	int yPosStart = chunkY * chunkSize;
	//loop over all positions for this chunk
	for (int x = 0; x <= chunkSize; ++x)
	{
		for (int y = 0; y <= chunkSize; ++y)
		{
			//add on current x and y position for current cube
			int xPos = xPosStart + x;
			int yPos = yPosStart + y;
			//get height at this position
			float baseNoiseValue = CalculateHeightFromNoise(xPos, yPos, noiseScale);

			int zPos;

			//add cube to correct terrain layer
			if (baseNoiseValue < WaterHeight)
			{
				//all water points are at the same height
				zPos = FMath::RoundToInt(WaterHeight * worldScale);
				WaterLayer.CubePositions.Add(FVector(xPos, yPos, zPos));
			}
			else
			{
				//round point to whole number
				zPos = FMath::RoundToInt(baseNoiseValue * worldScale);
				GrassLayer.CubePositions.Add(FVector(xPos, yPos, zPos));
			}
		}
	}
	
	//find border cubes to allow for seamless borders
	FindOuterBorderCubes(chunkSize, chunkX, chunkY, noiseScale);
	//find and fill the gaps in this chunk and between border points
	FillChunkGaps();
}

void AGenerator::FindOuterBorderCubes(int chunkSize, int chunkX, int chunkY, float noiseScale)
{
	//calculate xMin, x0, x1, xMax
	int x0 = chunkX * chunkSize;
	int xMin = x0 - 1;
	int x1 = chunkX * chunkSize + chunkSize;
	int xMax = x1 + 1;
	//calculate yMin, y0, y1, yMax
	int y0 = chunkY * chunkSize;
	int yMin = y0 - 1;
	int y1 = chunkY * chunkSize + chunkSize;
	int yMax = y1 + 1;

	//find border cubes along the x axis
	for (int i = x0; i < x1; ++i)
	{
		//cubes at yMin
		float baseNoiseValue = CalculateHeightFromNoise(i, yMin, noiseScale);
		int zPos = FMath::RoundToInt(baseNoiseValue * worldScale);
		borderCubePositions.Add(FVector(i, yMin, zPos));
		//cubes at yMax
		baseNoiseValue = CalculateHeightFromNoise(i, yMax, noiseScale);
		zPos = FMath::RoundToInt(baseNoiseValue * worldScale);
		borderCubePositions.Add(FVector(i, yMax, zPos));
	}

	//find border cubes along the y axis
	for (int i = y0; i < y1; ++i)
	{
		//cubes at xMin
		float baseNoiseValue = CalculateHeightFromNoise(xMin, i, noiseScale);
		int zPos = FMath::RoundToInt(baseNoiseValue * worldScale);
		borderCubePositions.Add(FVector(xMin, i, zPos));
		//cubes at xMax
		baseNoiseValue = CalculateHeightFromNoise(xMax, i, noiseScale);
		zPos = FMath::RoundToInt(baseNoiseValue * worldScale);
		borderCubePositions.Add(FVector(xMax, i, zPos));
	}
}

void AGenerator::FillChunkGaps()
{
	int minDepth = FMath::RoundToInt(WaterHeight * worldScale);

	TMap<FVector2D, int> allCubePositions;

	TArray<FVector> newCubePositions;

	TArray<FVector> allPositions = GrassLayer.CubePositions;
	allPositions.Append(WaterLayer.CubePositions);
	allPositions.Append(borderCubePositions);

	for (FVector pos : allPositions)
	{
		//add all points to map with key (x,y) position and value z position
		allCubePositions.Add(FVector2D(pos.X, pos.Y), (int)pos.Z);
	}

	for (FVector pos : GrassLayer.CubePositions)
	{
		int lowestPoint = INT_MAX;

		for (int nextX = -1; nextX <= 1; ++nextX)
		{
			for (int nextY = -1; nextY <= 1; ++nextY)
			{
				//excluded combinations
				if (FMath::Abs(nextX) + FMath::Abs(nextY) != 1) continue;
				if (!allCubePositions.Contains(FVector2D(pos.X + nextX, pos.Y + nextY))) continue;

				//get z position at (x,y) + offset
				int nextPoint = allCubePositions[FVector2D(pos.X + nextX, pos.Y + nextY)];
				//if less than current lowest neighbour point, replace it
				if (nextPoint < lowestPoint) lowestPoint = nextPoint;
			}
		}
		//smallest point should be minimum depth - water height
		if (lowestPoint < minDepth) lowestPoint = minDepth;

		if (lowestPoint < pos.Z)
		{
			int difference = pos.Z - lowestPoint;
			while (difference > 1) //if gap is greater than 1, there is that many cube sized gaps to fill
			{
				newCubePositions.Add(FVector(pos.X, pos.Y, lowestPoint + 1));
				difference--;
				lowestPoint++; //lowest point is now 1 higher
			}
		}
	}

	//add in the new positions that fill gaps in terrain
	GrassLayer.CubePositions.Append(newCubePositions);
	return;
}

float AGenerator::CalculateHeightFromNoise(int xPos, int yPos, float noiseScale)
{
	//get noise values at point in map
	float baseNoiseValue = FMath::PerlinNoise2D(FVector2D((xPos + seedX) * noiseScale, (yPos + seedY) * noiseScale));
	float mountainNoiseValue = FMath::PerlinNoise2D(FVector2D((xPos + seedX * 3) * noiseScale, (yPos + seedY * 3) * noiseScale));
	
	baseNoiseValue *= baseNoiseMultiplier;

	if (mountainNoiseValue > percentileNoise)
	{
		if (mountainNoiseValue < percentileTopNoise)
		{
			//point is within mountain range, so use interpolation to get smooth increase
			float t = (mountainNoiseValue - percentileNoise) / (percentileTopNoise - percentileNoise);
			float smoothT = CubicInterpolation(t);
			baseNoiseValue *= 1.0f + smoothT * MountainScale * 2.0f;
		}
		else
		{
			//point is at top of mountain range, so smoothen out as to not end in a sharp point
			baseNoiseValue *= 1.0f + 1.0f * MountainScale * 2.0f;
		}
	}

	return baseNoiseValue;
}

void AGenerator::CalculateLightingData()
{
	CalculateLayerLightingData(WaterLayer);
	CalculateLayerLightingData(GrassLayer);
}

void AGenerator::CalculateLayerLightingData(FTerrainLayer& layer)
{
	//check if layer has cubes
	if (layer.CubeCount == 0) return;

	if (useDetailedLighting)
	{
		//unique tangents and normals for every vertice and triangle 
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(layer.Vertices, layer.Triangles, TArray<FVector2D>(), layer.Normals, layer.Tangents);
		return;
	}

	//add vertice and triangles for first cube
	TArray<FVector> newVertices;
	TArray<int> newTriangles;
	for (int i = 0; i < 8; ++i)
	{
		newVertices.Add(layer.Vertices[i]);
	}
	for (int i = 0; i < 16; ++i)
	{
		newTriangles.Add(layer.Triangles[i]);
	}

	//calculate lighting info for first cube
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(newVertices, newTriangles, TArray<FVector2D>(), layer.UniversalNormals, layer.UniversalTangents);
	//apply to all cubes in this layer
	for (int i = 0; i < layer.CubeCount; ++i)
	{
		for (FVector n : layer.UniversalNormals)
		{
			layer.Normals.Add(n);
		}
		for (FProcMeshTangent t : layer.UniversalTangents)
		{
			layer.Tangents.Add(t);
		}
	}
}

void AGenerator::DrawTerrain()
{
	DrawTerrainSection(meshSectionsCount, WaterLayer, WaterMaterial);
	DrawTerrainSection(meshSectionsCount, GrassLayer, EarthMaterial);
}

void AGenerator::DrawTerrainSection(int meshSectionIndex, FTerrainLayer& layer, UMaterialInterface* material)
{
	//check if layer has cubes
	if (layer.CubeCount == 0) return;

	//draw mesh section & set terrain layer material
	ProceduralMesh->CreateMeshSection(meshSectionIndex, layer.Vertices, layer.Triangles, layer.Normals, TArray<FVector2D>(), TArray<FColor>(), layer.Tangents, false);
	ProceduralMesh->SetMaterial(meshSectionIndex, material);

	meshSectionsCount++;
}

float AGenerator::CubicInterpolation(float value)
{
	return value * value * (3.0 - 2.0 * value);
}

void AGenerator::PreGeneration(int chunkSize, int chunks, float noiseScale)
{
	//get noise map initial x and y pos from user seed
	seedX = (GenerationSeed * 6607 + 619823) % 1000000;
	seedY = (GenerationSeed * 2297 + 893153) % 1000000;

	if (MountainDensity == 0) return;

	TArray<float> values;

	//find all noise values
	for (int x = 0; x < chunks; ++x)
	{
		for (int y = 0; y < chunks; ++y)
		{
			for (int x2 = 0; x2 < chunkSize; ++x2)
			{
				for (int y2 = 0; y2 < chunkSize; ++y2)
				{
					int xPos = x * chunkSize + x2;
					int yPos = y * chunkSize + y2;

					float mountainNoiseValue = FMath::PerlinNoise2D(FVector2D((xPos + seedX * 3) * noiseScale, (yPos + seedX * 3) * noiseScale));

					values.Add(mountainNoiseValue);
				}
			}
		}
	}

	//order values
	values.HeapSort();
	//find noise percentile at user inputted % for mountain range
	float percentileIndex = ((100 - MountainDensity) / 100.0f) * (values.Num() - 1);
	percentileNoise = values[percentileIndex];
	//find noise percentile at top of mountain range for smoothening
	percentileIndex = ((100 - MountainDensity / 5) / 100.0f) * (values.Num() - 1);
	percentileTopNoise = values[percentileIndex];
}
