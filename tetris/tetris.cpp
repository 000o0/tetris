#include "stdafx.h"
#include "tetris.hpp"

// ENTRYPOINT
int main()
{
	auto my_console = console_controller(GetStdHandle(STD_OUTPUT_HANDLE));
	auto tetris_game = tetris(my_console, 30, 20);
	tetris_game.run();

	return 0;
}

// MAIN
void tetris::run()
{
	// PREPARE CONSOLE WINDOW
	this->console.initialise();
	this->console.set_title(L"TETRIS");

	// DRAW GAME BORDER, THIS IS WILL NOT BE TOUCHED
	// DURING GAME PLAY
	this->draw_boundary();

	// TETROMINO LIST IS FORMATTED AS: POSITION, IS_MOVING, TETROMINO 
	this->pieces.emplace_back(this->get_random_start_position(), true, this->get_random_tetromino());

	// GAME LOOP
	auto tick_count = GetTickCount64();
	while (true)
	{
		// CLEAR INSIDE OF FRAME EVERY TICK
		this->clear_game_frame();

		// ONLY MOVE CONTROLLABLE TETROMINO EVERY x ms
		auto move_piece = GetTickCount64() - tick_count > 50;

		// RESET TIMER WHEN MOVING TETROMINO
		if (move_piece)
			tick_count = GetTickCount64();

		auto add_new_piece = false;
		for (auto& [position, is_moving, comp] : this->pieces)
		{
			auto& [ref_position_x, ref_position_y] = position; // UNPACK POSITION

			this->draw_tetromino(ref_position_x, ref_position_y, comp);

			// MOVE TETROMINO DOWN ONCE TO CHECK FOR COLLISION
			auto position_x = ref_position_x;
			auto position_y = ref_position_y + 1;

			// CHECK COLLISION IF TETROMINO IS MOVING
			if (move_piece && is_moving)
			{
				// IF ANY FUTURE BLOCK COLLIDES, LOCK TETROMINO IN PLACE
				auto& parts = comp.get_elements();
				for (auto part : parts)
				{
					auto [part_x, part_y] = part;
					auto absolute_position = vector_t(position_x + part_x, position_y + part_y);

					// COLLIDED WITH ANOTHER SOLID TETROMINO?
					auto& solid_parts = this->solid_parts;
					auto piece_collision = std::find(solid_parts.begin(), solid_parts.end(), absolute_position) != solid_parts.end();
					
					// COLLIDED WITH BORDER?
					auto border_collision = absolute_position.second/*absolute y*/ >= this->border_height;

					// COLLISION!
					// LOCK TETROMINO IN PLACE AND SPAWN A NEW TETROMINO
					if (piece_collision || border_collision)
					{
						is_moving = false;
						add_new_piece = true;
						break;
					}
				}

				// ADD CURRENT TETROMINO TO SOLID PARTS
				if (add_new_piece)
				{
					for (auto new_part : parts)
					{
						auto[new_part_x, new_part_y] = new_part;
						auto new_absolute_position = vector_t(ref_position_x + new_part_x, ref_position_y + new_part_y);
						solid_parts.emplace_back(new_absolute_position);
					}
				}
				
				// MOVE TETROMINO
				if (is_moving)
					++ref_position_y;
			}
		}

		if (add_new_piece)
			this->pieces.emplace_back(this->get_random_start_position(), true, this->get_random_tetromino());

		Sleep(25);
	}
}

// DRAW BOUNDARIES
void tetris::draw_boundary()
{
	const auto border_character = L'█';
	for (int16_t i = 0; i <= this->border_height; i++)
		this->console.fill_horizontal(0, i, border_character, border_width);

	this->clear_game_frame();
}

// CLEAR INSIDE OF BORDER
void tetris::clear_game_frame()
{
	this->console.clear(1, 1, this->border_width - 2, this->border_height - 1);
}

// DRAW TETRIS PIECE, ALSO KNOWN AS TETROMINO, PART BY PART
void tetris::draw_tetromino(const int16_t x, const int16_t y, tetromino piece)
{
	for (size_t part_index = 0; part_index < piece.get_size(); part_index++)
	{
		auto[part_x, part_y] = piece[part_index];
		this->console.draw(x + part_x, y + part_y, piece.get_character());
	}
}

tetromino tetris::get_random_tetromino()
{
	// https://en.wikipedia.org/wiki/Tetris#Tetromino_colors
	static std::array<tetromino, 6> pieces =
	{
		// I TETROMINO
		// #
		// #
		// #
		// #
		tetromino('#',{ { 0, 0 },{ 0, -1 },{ 0, -2 },{ 0, -3 } }),

		// J TETROMINO
		// &&&
		//   &
		tetromino('&',{ { 0, 0 },{ 1, 0 },{ 2, 0 },{ 2, 1 } }),

		// L TETROMINO
		// @@@
		// @
		tetromino('@',{ { 0, 0 },{ 1, 0 },{ 2, 0 },{ 0, 1 } }),

		// O TETROMINO
		// $$
		// $$
		tetromino('$',{ { 0, 0 },{ 1, 0 },{ 0, 1 },{ 1, 1 } }),


		// T TETROMINO
		// +++
		//  +
		tetromino('+',{ { 0, 0 },{ 1, 0 },{ 2, 0 },{ 1, 1 } }),

		// Z TETROMINO
		// %%
		//  %%
		tetromino('%',{ { 0, 0 },{ 1, 0 },{ 1, 1 },{ 2, 1 } }),
	};


	return pieces.at(rng::get_int<size_t>(0, 5));
}

vector_t tetris::get_random_start_position()
{
	return vector_t(rng::get_int<int16_t>(4, this->border_width - 4), 4);
}
