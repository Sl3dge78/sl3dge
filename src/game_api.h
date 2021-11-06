typedef struct GameData GameData;

typedef u32 GameGetSize_t();
GameGetSize_t *pfn_GameGetSize;

typedef void GameInit_t(GameData *game_data, Renderer *renderer, PlatformAPI *platform_api);
GameInit_t *pfn_GameInit;

typedef void GameStart_t(GameData *game_data);
GameStart_t *pfn_GameStart;

typedef void GameEnd_t(GameData *game_data);
GameEnd_t *pfn_GameEnd;

typedef void GameLoop_t(float delta_time, GameData *game_data, Input *input);
GameLoop_t *pfn_GameLoop;
