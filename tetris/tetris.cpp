#include "stdafx.h"
#include "tetris.hpp"

// ENTRYPOINT
int main()
{
	const auto my_console = console_controller(GetStdHandle(STD_OUTPUT_HANDLE));
	auto tetris_game = tetris(my_console, 14, 20, '#');

	tetris_game.run();

	return 0;
}

// MAIN
void tetris::run()
{
	// PREPARE CONSOLE WINDOW
	this->console.set_title(L"TETRIS");

	// DRAW GAME BORDER, THIS IS WILL NOT BE TOUCHED
	// DURING GAME PLAY
	this->draw_boundary();

	// START GAME LOOP
	this->game_loop();
}

// DRAW BOUNDARIES
void tetris::draw_boundary()
{
	const auto border_character = this->piece_character;
	for (int16_t i = 0; i <= this->border_height; i++)
		this->console.fill_horizontal(0, i, border_character, border_width, 3);

	this->clear_game_frame();
}

// CLEAR INSIDE OF BORDER
void tetris::clear_game_frame()
{
	this->console.clear(1, 1, this->border_width - 2, this->border_height - 1);
}

// DRAW TETRIS PIECE, ALSO KNOWN AS TETROMINO, PART BY PART
void tetris::draw_tetromino(const screen_vector position, tetromino piece)
{
	auto[x, y] = position;

	SetConsoleTextAttribute(this->console.get_console_handle(), piece.get_color());

	for (size_t part_index = 0; part_index < piece.get_size(); part_index++)
	{
		const auto part = piece[part_index];
		this->console.draw(x + part.x, y + part.y, this->piece_character);
	}
}

tetromino tetris::get_random_tetromino()
{
	// https://en.wikipedia.org/wiki/Tetris#Tetromino_colors
	static std::array<tetromino, 6> pieces =
	{

		/*
		COLOR CODES
		10: GREEN
		11: CYAN
		12: RED
		13: PINK
		14: YELLOW
		15: WHITE
		*/

		/*
		I TETROMINO
		#
		#
		#
		#
		*/
		tetromino(10, { { 0, 0 },{ 0, 1 },{ 0, 2 },{ 0, 3 } }),

		/*
		J TETROMINO
		&&&
		  &
		*/
		tetromino(11,{ { 0, 0 },{ 1, 0 },{ 2, 0 },{ 2, 1 } }),

		/*
		L TETROMINO
		@@@
		@
		*/
		tetromino(12,{ { 0, 0 },{ 1, 0 },{ 2, 0 },{ 0, 1 } }),

		/*
		O TETROMINO
		$$
		$$
		*/
		tetromino(13,{ { 0, 0 },{ 1, 0 },{ 0, 1 },{ 1, 1 } }),


		/*
		T TETROMINO
		+++
		 +
		*/
		tetromino(14,{ { 0, 0 },{ -1, 0 },{ 1, 0 },{ 0, 1 } }),

		/*
		Z TETROMINO
		%%
		 %%
		*/
		tetromino(15,{ { 0, 0 },{ -1, 0 },{ 0, 1 },{ 1, 1 } }),
	};


	return pieces.at(rng::get_int<size_t>(0, 5));
}

screen_vector tetris::get_start_position()
{
	return screen_vector{ static_cast<int16_t>(this->border_width / 2), 1 };
}

void tetris::game_loop()
{
	auto update_move = std::chrono::steady_clock::now();

	auto moving_piece = tetromino_data(this->get_start_position(), true, this->get_random_tetromino());

	while (true)
	{
		// TIME AT BEGINNING OF LOOP TO CAP FRAMERATE AT PRECISELY 30 FPS
		const auto start_time = steady_clock_t::now();

		// CLEAR INSIDE OF FRAME EVERY TICK
		this->clear_game_frame();

		// ONLY MOVE CONTROLLABLE TETROMINO EVERY x ms
		const auto time_delta = std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock_t::now() - update_move);

		// RESET TIMER WHEN MOVING TETROMINO
		const auto should_move_piece = time_delta.count() > 250;
		if (should_move_piece)
			update_move = std::chrono::steady_clock::now();

		// DRAW TETROMINO AT CURRENT POSITION
		this->draw_tetromino(moving_piece.position, moving_piece.piece);

		// DRAW GHOST TETROMINO

		// CHECK IF TETROMINO IS MOVING
		if (moving_piece.is_moving)
			handle_moving_tetromino(moving_piece, should_move_piece);

		// ERASE ANY FULL LINE
		handle_full_lines();

		// DRAW SOLID PARTS BY ITERATING EACH ROW AND IT'S RESPECTIVE ELEMENTS
		draw_solid_parts();

		// SLEEP UNTIL ~33ms HAS PASSED FOR CONSISTENT 30 FPS
		std::this_thread::sleep_until(start_time + std::chrono::milliseconds(1000 / 30));
	}
}

void tetris::draw_solid_parts()
{
	for (size_t y = 0; y < this->solid_pieces.size(); y++)
	{
		for (size_t x = 0; x < this->solid_pieces[y].size(); x++)
		{
			const auto solid_piece = this->solid_pieces[y][x];
			if (solid_piece.valid)
				this->console.draw(x, y, this->piece_character, solid_piece.color_code);
		}
	}
}

void tetris::handle_full_lines()
{
	for (size_t y = 0; y < this->solid_pieces.size() - 1; y++)
	{
		const auto row_size = this->solid_pieces[y].size();

		bool full = true;
		for (size_t x = 1; x < row_size - 2; x++)
		{
			if (!this->solid_pieces[y][x].valid)
				full = false;
		}

		if (full)
		{
			// ERASE FULL LINE
			// this->solid_pieces[y] = std::vector<solid_piece>(row_size);

			// MOVE ALL LINES ABOVE IT DOWN
			for (size_t i = y; i > 2; i--)
			{
				this->solid_pieces[i] = this->solid_pieces[i - 1];
			}
		}
	}
}

void tetris::handle_moving_tetromino(tetromino_data& moving_piece, const bool& should_move_piece)
{
	// SET TO TRUE WHEN READY TO ADD A NEW PIECE
	// ONLY DO SO WHEN MOVING PIECE STOPS
	auto add_new_piece = false;

	// MOVE TETROMINO IF PLAYER TELLS TO
	this->handle_controls(moving_piece, add_new_piece);

	// MOVE TETROMINO DOWN ONCE EVERY x MS
	if (should_move_piece && moving_piece.is_moving)
		this->move_piece(moving_piece, add_new_piece);

	if (add_new_piece)
	{
		// LOCK MOVING PIECE IN PLACE
		this->add_solid_parts(moving_piece.piece, moving_piece.position);

		// ADD NEW PIECE IF MOVING PIECE WAS LOCKED IN PLACE
		moving_piece = tetromino_data(this->get_start_position(), true, this->get_random_tetromino());
	}
}

void tetris::add_solid_parts(tetromino& piece, const screen_vector& position)
{
	for (const auto part : piece.get_elements())
	{
		this->solid_pieces[position.y + part.y][position.x + part.x].color_code = piece.get_color();
		this->solid_pieces[position.y + part.y][position.x + part.x].valid = true;
	}
}

void tetris::handle_controls(tetromino_data& data, bool& add_new_piece)
{
	auto position_copy = data.position;

	const static key_handler_map_t key_handlers = {
		{
			// MOVE RIGHT
			VK_RIGHT, make_handler<std::plus<>>(&screen_vector::x)
			//[](tetris* instance, tetromino_data& data, screen_vector vector_copy, bool& add_new_piece)
			//{
			//	++vector_copy.x;
			//	if (!instance->does_element_collide(data.piece, vector_copy))
			//		++data.position.x;
			//}
		},
		{
			// MOVE LEFT
			VK_LEFT,
			[](tetris* instance, tetromino_data& data, screen_vector vector_copy, bool& add_new_piece)
			{
				--vector_copy.x;
				if (!instance->does_element_collide(data.piece, vector_copy))
					--data.position.x;
			}
		},
		{
			// MOVE DOWN
			VK_DOWN,
			[](tetris* instance, tetromino_data& data, screen_vector vector_copy, bool& add_new_piece)
			{
				++vector_copy.y;
				if (!instance->does_element_collide(data.piece, vector_copy))
					++data.position.y;
			}
		},
		{
			// ROTATE 90 DEGREES CLOCKWISE
			VK_UP,
			[](tetris* instance, tetromino_data& data, screen_vector vector_copy, bool& add_new_piece)
			{
				auto new_piece = data.piece.rotate(90.f);

				if (!instance->does_element_collide(new_piece, data.position))
					data.piece = new_piece;
			}
		},
		{
			// MOVE DOWN UNTIL COLLISION OCCURS
			VK_SPACE,
			[](tetris* instance, tetromino_data& data, screen_vector vector_copy, bool& add_new_piece)
			{

				auto collision = false;
				do
				{
					++vector_copy.y;

					collision = instance->does_element_collide(data.piece, vector_copy);

					if (!collision)
						++data.position.y;

				} while (!collision);

				data.is_moving = false;
				add_new_piece = true;
			},
		}
	};

	// ITERATE LIST OF HANDLERS AND INVOKE WHEN RESPECTIVE KEY IS PRESSED
	for (auto[vkey, handler_fn] : key_handlers)
	{
		if (console.get_key_press(vkey))
			handler_fn(this, data, position_copy, add_new_piece);
	}
}

void tetris::move_piece(tetromino_data& data, bool& add_new_piece)
{
	// MOVE TETROMINO DOWN ONCE TO CHECK FOR COLLISION
	auto copy_position = data.position;
	++copy_position.y;

	// IF ANY FUTURE BLOCK COLLIDES, LOCK TETROMINO IN PLACE
	if (!this->does_element_collide(data.piece, copy_position))
	{
		++data.position.y;
	}
	else
	{
		// LOCK TETROMINO IN PLACE
		data.is_moving = false;
		add_new_piece = true;
	}
}

bool tetris::does_element_collide(tetromino& piece, screen_vector position)
{
	auto& parts = piece.get_elements();
	for (auto part : parts)
	{
		// COLLISION! LOCK TETROMINO IN PLACE AND SPAWN A NEW TETROMINO
		if (this->collides(part, position))
			return true;
	}

	return false;
}

bool tetris::collides(screen_vector part, screen_vector position)
{
	const auto absolute_position = screen_vector{ position.x + part.x, position.y + part.y };

	// COLLIDED WITH A SOLID PIECE?
	auto piece_collision = this->solid_pieces[position.y + part.y][position.x + part.x].valid;

	// COLLIDED WITH BORDER?
	auto border_collision =
		absolute_position.y >= this->border_height ||		// COLLIDING WITH BOTTOM BORDER
		absolute_position.x < 1 ||							// COLLIDING WITH LEFT BORDER
		absolute_position.x > this->border_width - 2 ||		// COLLIDING WITH RIGHT BORDER
		absolute_position.y < 2;							// COLLIDING WITH TOP BORDER

	// RETURN TRUE EITHER WAY, ANY COLLISION SHALL HALT MOVEMENT
	return piece_collision || border_collision;
}
