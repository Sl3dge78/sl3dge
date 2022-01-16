#pragma once

typedef struct Enemy {
    i32 health;
    bool collided;
} Enemy;


void EnemyUpdate(Enemy *e, MeshHandle mesh, PushBuffer *pushbuffer) {

    /*
    // Draw
    if(e->health > 0) {
        PushMesh(pushbuffer, mesh, e->transform, (Vec3){1.0f, 0.0f, 0.0f});
    }
    */
}
