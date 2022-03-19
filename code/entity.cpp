global_variable b32 ViewedEntitiesCalculated;

void
CreateEntity(world* World, v2 Position, v2 Velocity, u32 Width, u32 Height, r32 Rotation, r32 FieldOfView, i32 ViewRange, entity_type Type, u32 Color)
{
    entity_storage* StorageToUpdate = World->EntityStorage;
    entity_storage* StorageToGetFrom = World->RemovedEntityStorage;

    if (StorageToUpdate->EntityCount == 0)
    {
        StorageToUpdate->Entities = (entity*)malloc(sizeof(entity));
    }

    // TODO: Fetching from the "StorageToGetFrom" 
    // should be better that this, I believe?
    entity NewEntity = {};
    u32 NewEntityID;
    if (StorageToGetFrom->EntityCount > 0)
    {
        NewEntity = StorageToGetFrom->Entities[--StorageToGetFrom->EntityCount];
        NewEntityID = NewEntity.ID;

        NewEntity.Component->P      = Position;
        NewEntity.Component->dP     = Velocity;
        NewEntity.Component->Width  = Width;
        NewEntity.Component->Height = Height;
        NewEntity.Component->Type   = Type;
        NewEntity.Component->Color  = Color;
        NewEntity.Component->RotationAngle  = Rotation;
        NewEntity.Component->FieldOfView    = FieldOfView;
        NewEntity.Component->ViewDistance   = ViewRange;

        NewEntity.Component->StorageIndex = StorageToUpdate->EntityCount;
    }
    else
    {
        NewEntityID = StorageToUpdate->EntityCount;
        entity_component* NewEntityComponent = (entity_component*)calloc(1, sizeof(entity_component));

        NewEntityComponent->P      = Position;
        NewEntityComponent->dP     = Velocity;
        NewEntityComponent->Width  = Width;
        NewEntityComponent->Height = Height;
        NewEntityComponent->Type   = Type;
        NewEntityComponent->Color  = Color;
        NewEntityComponent->RotationAngle   = Rotation;
        NewEntityComponent->FieldOfView     = FieldOfView;
        NewEntityComponent->ViewDistance    = ViewRange;

        NewEntity.Component = NewEntityComponent;

        NewEntity.ID = NewEntityID;

        NewEntity.Component->StorageIndex = StorageToUpdate->EntityCount;

        // NOTE: This is a bad thing here? 
        // I should think about a better solution here
        StorageToUpdate->Entities = (entity*)realloc(StorageToUpdate->Entities, sizeof(entity) * (StorageToUpdate->EntityCount + 1));
    }

    StorageToUpdate->Entities[StorageToUpdate->EntityCount++] = NewEntity;
}

// NOTE: Do I really want to remove Entities by their ID
// Maybe create another function to remove entities directly
void
RemoveEntityByID(world* World, u32 EntityID)
{
    entity_storage* StorageInUse = World->EntityStorage;
    entity_storage* StorageToStore = World->RemovedEntityStorage;

    if (StorageToStore->EntityCount == 0)
    {
        StorageToStore->Entities = (entity*)malloc(sizeof(entity));
    }

    entity EntityToRemove;
    entity EntityToMove = StorageInUse->Entities[--StorageInUse->EntityCount];

    if (EntityToMove.ID != EntityID) 
    {
        for (u32 EntityIndex = 0;
            EntityIndex < StorageInUse->EntityCount;
            ++EntityIndex)
        {
            entity Entity = StorageInUse->Entities[EntityIndex];
            if (Entity.ID == EntityID)
            {
                EntityToRemove = Entity;
                break;
            }
        }

        EntityToMove.Component->StorageIndex = EntityToRemove.Component->StorageIndex;
        EntityToRemove.Component->StorageIndex = StorageToStore->EntityCount;

        StorageInUse->Entities[EntityToMove.Component->StorageIndex] = EntityToMove;
    }
    else
    {
        EntityToRemove = EntityToMove;
    }

    // NOTE: This is a bad thing here? 
    // I should think about a better solution here
    StorageToStore->Entities = (entity*)realloc(StorageToStore->Entities, sizeof(entity) * (StorageToStore->EntityCount + 1));
    StorageToStore->Entities[StorageToStore->EntityCount++] = EntityToRemove;
}

entity*
GetEntityByType(world* World, entity_type Type)
{
    entity* Result = 0;

    entity_storage* StorageToUse = World->EntityStorage;
    for (u32 EntityIndex = 0;
        EntityIndex < StorageToUse->EntityCount;
        ++EntityIndex)
    {
        entity* EntityFound = StorageToUse->Entities + EntityIndex;
        if (EntityFound->Component->Type == Type)
        {
            Result = EntityFound;
        }
    }
    return Result;
}

struct collision_resolution_result
{
    v2 Normal;
    r32 Separation;
};

internal r32 
Distance(std::vector<v2> VerticesOfA, std::vector<v2> VerticesOfB)
{
    collision_resolution_result Result = {};

    u32 SizeOfA = VerticesOfA.size();
    u32 SizeOfB = VerticesOfB.size();

    r32 Separation = -FLT_MAX;
    for (u32 IndexA = 0;
        IndexA < SizeOfA;
        ++IndexA)
    {
        v2 SideOfA_A = VerticesOfA[IndexA];
        v2 SideOfA_B = VerticesOfA[(IndexA + 1) % SizeOfA];

        v2 NormalOfA = Normal(SideOfA_B - SideOfA_A);

        r32 MinimumSeparation = FLT_MAX;
        for (u32 IndexB = 0;
            IndexB < SizeOfB;
            ++IndexB)
        {
            v2 SideOfB_A = VerticesOfB[IndexB];

            r32 Projection = Inner(SideOfB_A - SideOfA_A, NormalOfA);
            if (Projection < MinimumSeparation)
            {
                MinimumSeparation = Projection;
            }
        }
        if (MinimumSeparation > Separation)
        {
            Separation = MinimumSeparation;
            Result.Normal = NormalOfA;
        }
    }

    return Separation;
}

internal b32
ResolveCollision(std::vector<v2> VerticesOfA, std::vector<v2> VerticesOfB)
{
    r32 DistanceA = Distance(VerticesOfA, VerticesOfB);
    r32 DistanceB = Distance(VerticesOfB, VerticesOfA);

    b32 Result = ((DistanceA < 0.0f) && (DistanceB < 0.0f));

    return Result;
}

// NOTE: This function used only at start up
internal b32
CalculateCollisionForEntity(world* World, v2 Pos, u32 Width, u32 Height)
{
    entity_storage* Storage = World->EntityStorage;
    b32 Result = false;

    std::vector<v2> Vertices;
    Vertices.push_back(Pos);
    Vertices.push_back(Pos + V2(Width, 0));
    Vertices.push_back(Pos + V2(Width, Height));
    Vertices.push_back((Pos + V2(0, Height)));

    for(u32 OtherEntityIndex = 0;
        OtherEntityIndex < Storage->EntityCount;
        ++OtherEntityIndex)
    {
        entity* OtherEntity  = Storage->Entities + OtherEntityIndex;

        std::vector<v2> VerticesOfOtherEntity;
        VerticesOfOtherEntity.push_back(OtherEntity->Component->P);
        VerticesOfOtherEntity.push_back(OtherEntity->Component->P + V2(OtherEntity->Component->Width, 0));
        VerticesOfOtherEntity.push_back(OtherEntity->Component->P + V2(OtherEntity->Component->Width, OtherEntity->Component->Height));
        VerticesOfOtherEntity.push_back((OtherEntity->Component->P + V2(0, OtherEntity->Component->Height)));

        if(ResolveCollision(Vertices, VerticesOfOtherEntity))
        {
            Result = true;
        }
    }

    return Result;
}

internal void
CalculateViewedEntitiesForSpecificEntity(entity* Entity, entity_storage* Storage)
{
    Entity->Component->AmmountOfViewedEntities = 0;

    v2 EntityCenter = Entity->Component->P + V2(Entity->Component->Width, Entity->Component->Height) / 2.0f;

#if _PDEBUG
    r32 RayAngle1 = Entity->Component->RotationAngle - (Entity->Component->FieldOfView / 2);
    r32 RayAngle2 = RayAngle1 + (Entity->Component->FieldOfView / 3);
    r32 RayAngle3 = RayAngle2 + (Entity->Component->FieldOfView / 3);

    v2 LineMax1 = EntityCenter + Entity->Component->ViewDistance * V2(cosf(RayAngle1), sinf(RayAngle1));
    v2 LineMax2 = EntityCenter + Entity->Component->ViewDistance * V2(cosf(RayAngle2), sinf(RayAngle2));
    v2 LineMax3 = EntityCenter + Entity->Component->ViewDistance * V2(cosf(RayAngle3), sinf(RayAngle3));

    std::vector<v2> VerticesOfEntity;
    VerticesOfEntity.push_back(EntityCenter);
    VerticesOfEntity.push_back(LineMax1);
    VerticesOfEntity.push_back(LineMax2);
    VerticesOfEntity.push_back(LineMax3);
#else
    std::vector<v2> VerticesOfEntity = GenerateFieldOfView(3, EntityCenter, Entity->Component->ViewDistance, Entity->Component->RotationAngle, Entity->Component->FieldOfView);
#endif

    for(u32 OtherEntityIndex = 0;
        OtherEntityIndex < Storage->EntityCount;
        ++OtherEntityIndex)
    {
        entity* OtherEntity  = Storage->Entities + OtherEntityIndex;
        if (Entity->ID == OtherEntity->ID) continue;

        std::vector<v2> VerticesOfOtherEntity;
        VerticesOfOtherEntity.push_back(OtherEntity->Component->P);
        VerticesOfOtherEntity.push_back(OtherEntity->Component->P + V2(OtherEntity->Component->Width, 0));
        VerticesOfOtherEntity.push_back(OtherEntity->Component->P + V2(OtherEntity->Component->Width, OtherEntity->Component->Height));
        VerticesOfOtherEntity.push_back((OtherEntity->Component->P + V2(0, OtherEntity->Component->Height)));

        if(ResolveCollision(VerticesOfEntity, VerticesOfOtherEntity))
        {
            Entity->Component->AmmountOfViewedEntities++;
        }
    }
}

void
UpdateEntities(world* World, r32 DeltaTime, bool* GameOver, i32* BallCount)
{
    entity_storage* StorageToUpdate = World->EntityStorage;
    for (u32 EntityIndex = 0;
        EntityIndex < StorageToUpdate->EntityCount;
        ++EntityIndex)
    {
        entity* Entity = StorageToUpdate->Entities + EntityIndex;

        Entity->Component->P += Entity->Component->dP * DeltaTime;

        if((Entity->Component->Type == EntityType_SelectedEntity) || Entity->Component->EntityRotated)
        {
            CalculateViewedEntitiesForSpecificEntity(Entity, StorageToUpdate);
        }

        Entity->Component->EntityRotated = false;
    }
}


void
SelectEntity(world* World, v2 MouseP)
{
    entity_storage* StorageToUse = World->EntityStorage;

    for (u32 EntityIndex = 0;
        EntityIndex < StorageToUse->EntityCount;
        ++EntityIndex)
    {
        entity* Entity = StorageToUse->Entities + EntityIndex;

        Entity->Component->Type = EntityType_OtherEntity;
        if(IsInRectangle(RectangleMinDim(Entity->Component->P, V2(Entity->Component->Width, Entity->Component->Height)), MouseP))
        {
            Entity->Component->Type = EntityType_SelectedEntity;
        }
    }
}
