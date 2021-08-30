typedef struct GameData GameData;
typedef struct Renderer Renderer;
typedef struct Input Input;
typedef struct PlatformAPI PlatformAPI;

typedef u32 GameGetSize_t();
GameGetSize_t *pfn_GameGetSize;

typedef void GameLoad_t(GameData *game_data, Renderer *renderer, PlatformAPI *platform_api);
GameLoad_t *pfn_GameLoad;

typedef void GameStart_t(GameData *game_data);
GameStart_t *pfn_GameStart;

typedef void GameLoop_t(float delta_time, GameData *game_data, Input *input);
GameLoop_t *pfn_GameLoop;
