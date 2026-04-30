#include "HexTile.h"

bool UHexTile::CanEnemyTraverse() const
{
	return TileType == EHexTileType::Road;
}

bool UHexTile::CanAllyTraverse() const
{
	return TileType != EHexTileType::Block;
}

bool UHexTile::CanBuildTower() const
{
	return TileType == EHexTileType::Tower;
}
