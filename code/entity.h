#if !defined(ENTITY_H_)

enum entity_type
{
    EntityType_SelectedEntity,
    EntityType_OtherEntity,

    EntityType_Count,
};

// NOTE: I could try here to create
// a second entity storage for removed entity ids
// instead of this thing.
// But Could it be cost more on modern machines?
enum entity_flags
{
    EntityFlag_Placed = (1 << 0),
};

struct entity_component
{
    v2 P;
    v2 dP;

    u32 Width;
    u32 Height;

    u32 Color;

    b32 Flags;
    b32 EntityRotated;

    i32 StorageIndex;

    r32 RotationAngle;

    r32 FieldOfView;
    i32 ViewDistance;
    i32 AmmountOfViewedEntities;

    entity_type Type;
};

struct entity
{
    u32 ID;
    entity_component* Component;
};

struct entity_storage
{
    entity* Entities;
    u32 EntityCount;
};

struct world
{
    entity_storage* EntityStorage;
    entity_storage* RemovedEntityStorage;

    // NOTE: Should I count entities that is in use?
    // Entities that I got from this storage?
    // But isn't too space consuming?
    entity GlobalEntityStorage[512];
};

void CreateEntity(world* World, v2 Position, v2 Velocity, u32 Width, u32 Height, r32 Rotation, r32 FieldOfView, i32 ViewRange, entity_type Type, u32 Color = 0xFFFFFFFF);
void RemoveEntityByID(world* World, u32 EntityID);
entity* GetEntityByType(world* World, entity_type Type);
b32 ResolveCollision(std::vector<v2> A, std::vector<v2> B);
b32 CalculateCollisionForEntity(world* World, v2 Pos, u32 Width, u32 Height);
void UpdateEntities(world* World, r32 DeltaTime, bool* GameOver = nullptr, i32* BallCount = 0);
void SelectEntity(world* World, v2 MouseP);

#define ENTITY_H_ 
#endif
