#include "SpriteManager.h"

#include "Scroll.h"

#include "SpritePrincess.h"
#include "SpriteZurrapa.h"
#include "SpriteParticle.h"
#include "SpriteAxe.h"

#include "BankManager.h"

#include <string.h>

//Pool
struct Sprite sprite_manager_sprites[N_SPRITE_MANAGER_SPRITES];
DECLARE_STACK(sprite_manager_sprites_pool, N_SPRITE_MANAGER_SPRITES);

//Current sprites
DECLARE_VECTOR(sprite_manager_updatables, N_SPRITE_MANAGER_SPRITES);

UINT8 sprite_manager_removal_check;

void SpriteManagerReset() {
	UINT8 i;

	//place all sprites on the pool
	sprite_manager_sprites_pool[0] = N_SPRITE_MANAGER_SPRITES;
	for(i = 0; i != N_SPRITE_MANAGER_SPRITES; ++i) {
		sprite_manager_sprites_pool[i + 1] = i;

		sprite_manager_sprites[i].oam_idx = i << 1;
		move_sprite(i << 1, 200, 200);
		move_sprite((i << 1) + 1, 200, 200);
	}

	//Clear the list of updatable sprites
	sprite_manager_updatables[0] = 0;
	sprite_manager_removal_check = 0;
}

struct Sprite* SpriteManagerAdd(SPRITE_TYPE sprite_type) {
	UINT8 sprite_idx;
	struct Sprite* sprite;

	sprite_idx = StackPop(sprite_manager_sprites_pool);
	sprite = &sprite_manager_sprites[sprite_idx];
	sprite->type = sprite_type;
	sprite->marked_for_removal = 0;
	sprite->lim_x = 32u;
	sprite->lim_y = 32u;

	VectorAdd(sprite_manager_updatables, sprite_idx);

	PUSH_BANK(2);
	switch((SPRITE_TYPE)sprite->type) {
		case SPRITE_TYPE_PRINCESS:      StartPrincess(sprite); break;
		case SPRITE_TYPE_ZURRAPA:       StartZurrapa(sprite);  break;
		case SPRITE_TYPE_DEAD_PARTICLE: StartParticle(sprite); break;
		case SPRITE_TYPE_AXE:           StartAxe(sprite);      break;
	}
	POP_BANK;

	return sprite;
}

void SpriteManagerRemove(int idx) {
	sprite_manager_removal_check = 1;
	sprite_manager_sprites[sprite_manager_updatables[idx + 1]].marked_for_removal = 1;
}

void SpriteManagerRemoveSprite(struct Sprite* sprite) {
	UINT8 i;
	struct Sprite* s;
	for(i = 0u; i != sprite_manager_updatables[0]; ++i) {
		s = &sprite_manager_sprites[sprite_manager_updatables[i + 1]];
		if(s == sprite) {
			SpriteManagerRemove(i);
			break;
		}
	}
}

UINT8 sprite_manager_current_index;
struct Sprite* sprite_manager_current_sprite;
void SpriteManagerUpdate() {
	for(sprite_manager_current_index = 0u; sprite_manager_current_index != sprite_manager_updatables[0]; ++sprite_manager_current_index) {
		sprite_manager_current_sprite = &sprite_manager_sprites[sprite_manager_updatables[sprite_manager_current_index + 1]];
		if(!sprite_manager_current_sprite->marked_for_removal) {
			PUSH_BANK(2);
			switch((SPRITE_TYPE)sprite_manager_current_sprite->type) {
				case SPRITE_TYPE_PRINCESS:      UpdatePrincess(); break;
				case SPRITE_TYPE_ZURRAPA:       UpdateZurrapa();  break;
				case SPRITE_TYPE_DEAD_PARTICLE: UpdateParticle(); break;
				case SPRITE_TYPE_AXE:           UpdateAxe();      break;
			}
			POP_BANK;

			if( ((scroll_x - sprite_manager_current_sprite->x - 16u - sprite_manager_current_sprite->lim_x)          & 0x8000u) &&
			    ((sprite_manager_current_sprite->x - scroll_x - SCREENWIDTH - sprite_manager_current_sprite->lim_x)  & 0x8000u) &&
					((scroll_y - sprite_manager_current_sprite->y - 16u - sprite_manager_current_sprite->lim_y)          & 0x8000u) &&
					((sprite_manager_current_sprite->y - scroll_y - SCREENHEIGHT - sprite_manager_current_sprite->lim_y) & 0x8000u)
			) { 
				DrawSprite(sprite_manager_current_sprite);
			} else {
				SpriteManagerRemove(sprite_manager_current_index);
			}
		}
	}

	if(sprite_manager_removal_check) {
		//We must remove sprites in inverse order because everytime we remove one the vector shrinks and displaces all elements
		for(sprite_manager_current_index = sprite_manager_updatables[0] - 1; sprite_manager_current_index + 1 != 0u; sprite_manager_current_index -= 1u) {
			sprite_manager_current_sprite = &sprite_manager_sprites[sprite_manager_updatables[sprite_manager_current_index + 1u]];
			if(sprite_manager_current_sprite->marked_for_removal) {
				StackPush(sprite_manager_sprites_pool, sprite_manager_updatables[sprite_manager_current_index + 1u]);
				VectorRemovePos(sprite_manager_updatables, sprite_manager_current_index);
				move_sprite(sprite_manager_current_sprite->oam_idx, 200, 200);
				move_sprite(sprite_manager_current_sprite->oam_idx + 1, 200, 200);
			}
		}
		sprite_manager_removal_check = 0;
	}
}
