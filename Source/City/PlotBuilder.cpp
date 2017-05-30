// Fill out your copyright notice in the Description page of Project Settings.

#include "City.h"
#include "simplexnoise.h"
#include "PlotBuilder.h"



// Sets default values
APlotBuilder::APlotBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void APlotBuilder::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector getPointDirection(const FPolygon &pol, int place, bool left) {
	if (place == 0) {
		FVector dir = pol.points[1] - pol.points[0];
		dir.Normalize();
		return FRotator(0, left ? 90 : 270, 0).RotateVector(dir);
	}
	else if (place == pol.points.Num() - 1) {
		FVector dir = pol.points[pol.points.Num() - 1] - pol.points[pol.points.Num() - 2];
		dir.Normalize();
		return FRotator(0, left ? 90 : 270, 0).RotateVector(dir);
	}
	else {
		FVector dir1 = pol.points[place] - pol.points[place-1];
		FVector dir2 = pol.points[place+1] - pol.points[place];
		dir1.Normalize();
		dir2.Normalize();
		dir1 = FRotator(0, left ? 90 : 270, 0).RotateVector(dir1);
		dir2 = FRotator(0, left ? 90 : 270, 0).RotateVector(dir2);

		FVector totDir = dir1 + dir2;
		totDir.Normalize();
		return totDir;
	}

}


TArray<FHousePolygon> APlotBuilder::generateHousePolygons(FPlotPolygon p, TArray<FPolygon> others, int maxFloors, int minFloors) {
	TArray<FHousePolygon> housePolygons;

	float maxArea = 4000.0f;
	float minArea = 800.0f;

	if (!p.open) {


		FHousePolygon original;
		original.points = p.points;
		if (!p.buildLeft)
			original.reverse();
		//original.checkOrientation();
		//original.buildLeft = p.buildLeft;
		original.buildLeft = true;
		original.open = false;
		original.population = p.population;
		original.type = p.type;
		for (int32 i = 1; i < original.points.Num(); i++) {
			original.entrances.Add(i);
			original.windows.Add(i);
		}


		TArray<FHousePolygon> refinedPolygons = original.refine(maxArea, minArea, 0);
		for (FHousePolygon r : refinedPolygons) {
			r.height = randFloat() * (maxFloors - minFloors) + minFloors;
			r.type = randFloat() < 0.5 ? RoomType::office : RoomType::apartment;

			float area = r.getArea();
			UE_LOG(LogTemp, Log, TEXT("area of new house polygon: %f"), area);

			//if (area > minArea) {
			housePolygons.Add(r);
			others.Add(r);
		}

	}
	else {
		// wander along the line and place adjacent houses on the curve

		float minLen = 4000;
		float minWidth = 5000;

		float maxLen = 6000;
		float maxWidth = 6000;

		float acceptableLen = 8000;

		int next = 1;
		FVector prevTan = FVector(0, 0, 0);
		FVector prev1 = FVector(0, 0, 0);

		FVector currPos = p.points[0];
		//FPolygon* activePol = nullptr;
		//TArray<FVector> toApply;
		//while (next < p.points.Num()) {
		//	FVector currPos1 = p.points[next - 1];
		//	FVector dir1 = getPointDirection(p, next - 1, p.buildLeft);
		//	FVector dir2 = p.points[next] - p.points[next - 1];
		//	dir2.Normalize();
		//	float width = FMath::FRandRange(minWidth, maxWidth);
		//	float len = FMath::FRandRange(minLen, maxLen);
		//	FVector currPos2 = currPos1 + dir1 * width;
		//	FVector currPos3 = p.points[next];
		//	FVector currPos4 = p.points[next] + getPointDirection(p, next, p.buildLeft) * width;

		//	FHousePolygon pol;
		//	pol.points.Add(currPos1);
		//	pol.points.Add(currPos3);
		//	pol.points.Add(currPos4);
		//	pol.points.Add(currPos2);
		//	pol.points.Add(currPos1);
		//	pol.housePosition = pol.getCenter();
		//	pol.checkOrientation();
		//	pol.population = 1.0;
		//	pol.height = randFloat() * (maxFloors - minFloors) + minFloors;
		//	pol.housePosition = pol.getCenter();

		//	if (p.buildLeft) {
		//		pol.entrances.Add(1);
		//		pol.windows.Add(1);
		//	}
		//	else {
		//		pol.entrances.Add(4);
		//		pol.windows.Add(4);
		//	}

		//	housePolygons.Add(pol);

		//	next++;

		//	//if (!activePol) {
		//	//	activePol = &FPolygon();
		//	//}
		//}
		while (next < p.points.Num()) {
			if (FVector::Dist(p.points[next], currPos) < minLen) {
				currPos = p.points[next];
				next++;
				prev1 = FVector(0.0f, 0.0f, 0.0f);
				prevTan = FVector(0.0f, 0.0f, 0.0f);
				continue;
			}
			FHousePolygon fh;
			FVector tangent1 = p.points[next] - currPos;
			tangent1.Normalize();
			FPolygon pol;
			float len = 0;
			if (p.buildLeft) {
				FVector tangent2 = FRotator(0, 90, 0).RotateVector(tangent1);
				if (FVector::Dist(p.points[next], currPos) < acceptableLen)
					len = FVector::Dist(p.points[next], currPos);
				else
					len = std::min(FVector::Dist(p.points[next], currPos), randFloat()*(maxLen - minLen) + minLen);
				float width = randFloat()*(maxWidth - minWidth) + minWidth;
				if (prev1.X != 0.0f) {
					pol.points.Add(prev1 + prevTan * width);
					pol.points.Add(prev1);
				}
				else {
					pol.points.Add(currPos + width*tangent2);
					pol.points.Add(currPos);
				}
				pol.points.Add(currPos + len*tangent1);
				pol.points.Add(currPos + len*tangent1 + width*tangent2);
				FVector first = pol.points[0];
				pol.points.Add(first);
			}
			else {
				FVector tangent2 = FRotator(0, 270, 0).RotateVector(tangent1);
				if (FVector::Dist(p.points[next], currPos) < acceptableLen)
					len = FVector::Dist(p.points[next], currPos);
				else
					len = std::min(FVector::Dist(p.points[next], currPos), randFloat()*(maxLen - minLen) + minLen);

				float width = randFloat()*(maxWidth - minWidth) + minWidth;

				pol.points.Add(currPos + len*tangent1 + width*tangent2);
				pol.points.Add(currPos + len*tangent1);
				if (prev1.X != 0.0f) {
					pol.points.Add(prev1);
					pol.points.Add(prev1 + prevTan * width);
				}
				else {
					pol.points.Add(currPos);
					pol.points.Add(currPos + width*tangent2);
				}

				FVector first = pol.points[0];
				pol.points.Add(first);
			}
			FPolygon tmp;
			
 			if (!testCollision(pol, others, 500, tmp)) {
				fh.points = pol.points;
				fh.checkOrientation();
				fh.population = 1.0;
				fh.height = randFloat() * (maxFloors - minFloors) + minFloors;
				fh.housePosition = pol.getCenter();
				fh.type = raw_noise_2d(fh.housePosition.X, fh.housePosition.Y) < 0 ? RoomType::office : RoomType::apartment;//randFloat() < 0.5 ? RoomType::office : RoomType::apartment;

				fh.entrances.Add(2);
				for (int i = 1; i < fh.points.Num(); i++) {
					fh.windows.Add(i);
				}
				others.Add(fh);
				housePolygons.Add(fh);
				FVector tangent = pol.points[2] - pol.points[1];
				tangent.Normalize();
				prev1 = pol.points[2] + tangent;
				prevTan = pol.points[3] - pol.points[2];
				prevTan.Normalize();

			}


			currPos += len*tangent1;

		}

	}
	return housePolygons;

}

FPolygon APlotBuilder::generateSidewalkPolygon(FPlotPolygon p, float offsetSize) {
	FPolygon polygon;
		FVector center = p.getCenter();
		FPolygon sidewalk;
		for (int i = 1; i < p.points.Num(); i+=1) {
			FVector tangent = p.points[i] - p.points[i - 1];
			tangent.Normalize();
			FVector offset = (p.buildLeft ? FRotator(0, 270, 0) : FRotator(0, 90, 0)).RotateVector(tangent * offsetSize);
			polygon.points.Add(p.points[i - 1] + offset);
			polygon.points.Add(p.points[i] + offset);
		}
		if (!p.open) {
			FVector tangent = p.points[1] - p.points[0];
			tangent.Normalize();
			FVector offset = (p.buildLeft ? FRotator(0, 270, 0) : FRotator(0, 90, 0)).RotateVector(tangent * offsetSize);
			polygon.points.Add(p.points[0] + offset);
			polygon.points.Add(p.points[1] + offset);
			
		}
		else {
			FVector last = p.points[p.points.Num() - 1];
			polygon.points.Add(last);
			polygon.points.Add(last);

		}
	return polygon;
}

FSidewalkInfo APlotBuilder::getSideWalkInfo(FPolygon sidewalk)
{
	FSidewalkInfo toReturn;
	int toPlace = 6;
	if (FMath::FRand() < 0.1f) {
		for (int i = 1; i < sidewalk.points.Num(); i += 2) {
			for (int j = 1; j < toPlace; j++) {
				FVector origin = sidewalk.points[i - 1];
				FVector target = sidewalk.points[i];
				FVector tan = target - origin;
				float len = tan.Size();
				tan.Normalize();
				toReturn.staticMeshes.Add(FMeshInfo{ "tree", FTransform(origin + j * tan * (len / toPlace)) });
			}
		}
	}


	return toReturn;
}



// Called every frame
void APlotBuilder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

