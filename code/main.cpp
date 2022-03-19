#include <memory>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <random>
#include <math.h>
#include <SDL2/SDL.h>

#include "display.h"
#include "entity.cpp"

bool is_running;
bool IsDebug = false;
bool StartGame = false;

i32 PreviousFrameTime = 0;
r32 DeltaTime = 0;
r32 TimeForFrame = 0;
r32 dtForFrame = 0;

internal void
CreateWorld(world* World, u32 NumOfEntities)
{
#if 0
    i32 EntityWidth  = ColorBuffer->Width  / NumOfEntities;
    i32 EntityHeight = ColorBuffer->Height / NumOfEntities;
#else
    i32 EntityWidth  = 8;
    i32 EntityHeight = 8;
#endif

    b32 NewPositionFailed = false;
    for(u32 EntityIndex = 0;
        EntityIndex < NumOfEntities;
        ++EntityIndex)
    {
        for(u32 AmmountOfFails = 0;
            AmmountOfFails < 5;
            ++AmmountOfFails)
        {
            NewPositionFailed = false;

            i32 X = rand() % ColorBuffer->Width;
            i32 Y = rand() % ColorBuffer->Height;

            v2 EntityPos = V2(X, Y);

            if(CalculateCollisionForEntity(World, EntityPos, EntityWidth, EntityHeight))
            {
                NewPositionFailed = true;
            }

            if(!NewPositionFailed)
            {
                CreateEntity(World, EntityPos, V2(0, 0), 
                             EntityWidth, EntityHeight, 
                             AngleToRadians(rand() % 360), // Player Rotation Angle
                             AngleToRadians(rand() % (270 + 1 - 30) + 30), // Field Of View Angle
                             rand() % ((ColorBuffer->Width / 2) + 1 - 100) + 100, // View Distance
                             EntityType_OtherEntity);
                break;
            }
        }
    }
}

internal void 
setup(world* World)
{

    ColorBuffer->Memory = (u32*)malloc(sizeof(u32)*ColorBuffer->Width*ColorBuffer->Height);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
                                SDL_TEXTUREACCESS_STREAMING, 
                                ColorBuffer->Width, ColorBuffer->Height);

    CreateWorld(World, 100);
}

internal void 
process_input(world* World)
{
    SDL_Event event;
    SDL_PollEvent(&event);
    SDL_PumpEvents();

    entity* Entity = GetEntityByType(World, EntityType_SelectedEntity);

    i32 MouseX, MouseY;
    i32 MouseButton = SDL_GetMouseState(&MouseX, &MouseY);
    v2 MouseP = V2(MouseX, MouseY);

    b32 IsEnterDown  = false;
    b32 IsLeftMouse  = MouseButton & SDL_BUTTON(SDL_BUTTON_LEFT);
    b32 IsRightMouse = MouseButton & SDL_BUTTON(SDL_BUTTON_RIGHT);

    switch(event.type)
    {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_KEYDOWN:
            if(event.key.keysym.sym == SDLK_ESCAPE) is_running = false;
            if(event.key.keysym.sym == SDLK_r) IsDebug = !IsDebug;
            if(event.key.keysym.sym == SDLK_RETURN)
            {
                IsEnterDown = true;
            }
            break;
        case SDL_KEYUP:
            break;
    }

    if(IsEnterDown)
    {
        World->EntityStorage->EntityCount = 0;
        World->RemovedEntityStorage->EntityCount = 0;

        i32 MaxEntities = (2 ^ 10);
        i32 MinEntities = 32;
        CreateWorld(World, rand() % (MaxEntities + 1 - MinEntities) + MinEntities);
    }
    if(IsLeftMouse)
    {
        SelectEntity(World, V2(MouseX, MouseY));
    }
    if(IsRightMouse)
    {
        entity* EntityToModify = GetEntityByType(World, EntityType_SelectedEntity);

        v2 Pos = Entity->Component->P;
        r32 SideA = Pos.x - MouseP.x;
        r32 SideB = Pos.y - MouseP.y;
        r32 NewAngle = SideB / SideA;

        EntityToModify->Component->RotationAngle = NewAngle;
        EntityToModify->Component->EntityRotated = true;
    }
}

internal void 
update(world* World)
{
    dtForFrame += TimeForFrame;
    int TimeToWait = FRAME_TARGET_TIME - (SDL_GetTicks() + PreviousFrameTime);

    if(TimeToWait > 0 && (TimeToWait <= FRAME_TARGET_TIME))
    {
        SDL_Delay(TimeToWait);
    }

    DeltaTime = (SDL_GetTicks() - PreviousFrameTime) / 1000.0f;
    //if (DeltaTime > TimeForFrame) DeltaTime = TimeForFrame;
    PreviousFrameTime = SDL_GetTicks();

    UpdateEntities(World, DeltaTime);
}

internal void 
render(world* World)
{
    RenderColorBuffer();
    ClearColorBuffer(ColorBuffer, 0xFF16161d);//0xFF056263);

    i32 MouseX, MouseY;
    SDL_GetMouseState(&MouseX, &MouseY);

    entity_storage* StorageToUpdate = World->EntityStorage;
    for(u32 EntityIndex = 0;
        EntityIndex < StorageToUpdate->EntityCount;
        ++EntityIndex)
    {
        entity* Entity = StorageToUpdate->Entities + EntityIndex;

        v2 Start  = Entity->Component->P;
        v2 Width  = Entity->Component->Width  * V2(1, 0);
        v2 Height = Entity->Component->Height * V2(0, 1);

        switch(Entity->Component->Type)
        {
            case EntityType_OtherEntity:
            {
                DrawRotRect(ColorBuffer, Start, Width, Height, 0xFF0000FF, nullptr);
                if(IsInRectangle(RectangleMinDim(Start, V2(Width.x, Height.y)), V2(MouseX, MouseY)))
                {
                    v2 DebugStart = Start;

                    DrawOutline(ColorBuffer, DebugStart, Width, Height, 1);
                }
            } break;

            case EntityType_SelectedEntity:
            {
                DrawRotRect(ColorBuffer, Start, Width, Height, 0xFF0000FF, nullptr);

                DrawFieldOfView(Entity->Component->RotationAngle, Start + V2(Width.x, Height.y) / 2.0f, Entity->Component->FieldOfView, Entity->Component->ViewDistance, 0xFFFF00FF);

                printf("Entity ID: %i\n Viewed Entities Count: %i\n", Entity->ID, Entity->Component->AmmountOfViewedEntities);
                fflush(stdout);
            } break;
        }
    }

    SDL_RenderPresent(renderer);
}

int 
main(int argc, char** argv)
{
    is_running = InitWindow();

    srand(time(NULL));

    world* World = (world*)malloc(sizeof(world));
    World->EntityStorage = (entity_storage*)calloc(1, sizeof(entity_storage));
    World->RemovedEntityStorage = (entity_storage*)calloc(1, sizeof(entity_storage));

    setup(World);

    while(is_running)
    {
        process_input(World);
        update(World);
        render(World);
    }

    DestroyWindow();

    return 0;
}
