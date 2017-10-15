#include <Windows.h>
#include <stdlib.h>

#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;

enum class FieldElement : char
{
    Empty,
    Tetronimo_0,
    Tetronimo_1,
    Tetronimo_2,
    Tetronimo_3,
    Tetronimo_4,
    Tetronimo_5,
    Tetronimo_6,
    CompleteLine
};

const int field_width = 10;
const int field_height = 20;
const int field_x_offset = 8;
const int field_y_offset = 8;
const int screen_width = field_width + field_x_offset * 2;
const int screen_height = field_height + field_y_offset * 2;
const int update_delay = 50;

HANDLE console = INVALID_HANDLE_VALUE;
CHAR_INFO screen_buffer[screen_width * screen_height];
wstring tetronimos[7];
FieldElement field[field_width * field_height] = { FieldElement::Empty };
int force_down_time = 1000;

int rotate(int px, int py, int r)
{
    if (r == 0)
    {
        // 0-degrees
        return py * 4 + px;
    }
    else if (r == 1)
    {
        // 90-degrees
        return py + (3 - px) * 4;
    }
    else if (r == 2)
    {
        // 180-degrees
        return 15 - py * 4 - px;
    }
    else if (r == 3)
    {
        // 270-degrees
        return 15 - py - (3 - px) * 4;
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

bool valid_position(int t, int x, int y, int r)
{
    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            if (tetronimos[t][rotate(px, py, r)] == L'X')
            {
                if (x + px < 0 || x + px >= field_width || y + py < 0 || y + py >= field_height || field[x + px + (y + py) * field_width] != FieldElement::Empty)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool can_drop(int y)
{
    for (int x = 0; x < field_width; ++x)
    {
        if (field[x + y * field_width] != FieldElement::Empty && field[x + (y + 1) * field_width] != FieldElement::Empty)
        {
            return false;
        }
    }

    return true;
}

bool drop_lines(int start_y)
{
    bool dropped_any = false;

    for (int y = start_y; y >= 0; --y)
    {
        for (int x = 0; x < field_width; ++x)
        {
            if (field[x + y * field_width] != FieldElement::Empty)
            {
                dropped_any = true;
                field[x + (y + 1) * field_width] = field[x + y * field_width];
                field[x + y * field_width] = FieldElement::Empty;
            }
        }
    }

    return dropped_any;
}

void find_full_lines(int start_y, int end_y, vector<int>& full_lines)
{
    for (int y = start_y; y < end_y; ++y)
    {
        bool full = true;

        for (int x = 0; x < field_width; ++x)
        {
            FieldElement element = field[x + y * field_width];
            if (element < FieldElement::Tetronimo_0 || element > FieldElement::Tetronimo_6)
            {
                full = false;
                break;
            }
        }

        if (full)
        {
            full_lines.push_back(y);
        }
    }
}

bool key_down(int vk_key)
{
    return (GetAsyncKeyState(vk_key) & 0x8000) == 0x8000;
}

int main()
{
    // Setup the console
    console = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!console)
    {
        exit(EXIT_FAILURE);
    }

    SMALL_RECT console_window = { 0, 0, 1, 1 };

    if (!SetConsoleWindowInfo(console, TRUE, &console_window))
    {
        exit(EXIT_FAILURE);
    }

    COORD buffer_size = { screen_width, screen_height };

    if (!SetConsoleScreenBufferSize(console, buffer_size))
    {
        exit(EXIT_FAILURE);
    }

    if (!SetConsoleActiveScreenBuffer(console))
    {
        exit(EXIT_FAILURE);
    }

    CONSOLE_FONT_INFOEX font_info = {};
    font_info.cbSize = sizeof(font_info);
    font_info.nFont = 0;
    font_info.dwFontSize.X = 12;
    font_info.dwFontSize.Y = 12;
    font_info.FontFamily = FF_DONTCARE;
    font_info.FontWeight = FW_DONTCARE;
    wcscpy_s(font_info.FaceName, L"Consolas");

    if (!SetCurrentConsoleFontEx(console, FALSE, &font_info))
    {
        exit(EXIT_FAILURE);
    }

    console_window = { 0, 0, (SHORT)screen_width - 1, (SHORT)screen_height - 1 };

    if (!SetConsoleWindowInfo(console, TRUE, &console_window))
    {
        exit(EXIT_FAILURE);
    }

    CONSOLE_CURSOR_INFO cursor_info = {};
    cursor_info.dwSize = sizeof(cursor_info);

    if (!SetConsoleCursorInfo(console, &cursor_info))
    {
        exit(EXIT_FAILURE);
    }

    // Setup the Tetronimos
    tetronimos[0].append(L"  X ");
    tetronimos[0].append(L"  X ");
    tetronimos[0].append(L"  X ");
    tetronimos[0].append(L"  X ");

    tetronimos[1].append(L"  X ");
    tetronimos[1].append(L" XX ");
    tetronimos[1].append(L" X  ");
    tetronimos[1].append(L"    ");

    tetronimos[2].append(L" X  ");
    tetronimos[2].append(L" XX ");
    tetronimos[2].append(L"  X ");
    tetronimos[2].append(L"    ");

    tetronimos[3].append(L"    ");
    tetronimos[3].append(L" XX ");
    tetronimos[3].append(L" XX ");
    tetronimos[3].append(L"    ");

    tetronimos[4].append(L"  X ");
    tetronimos[4].append(L" XX ");
    tetronimos[4].append(L"  X ");
    tetronimos[4].append(L"    ");

    tetronimos[5].append(L" XX ");
    tetronimos[5].append(L"  X ");
    tetronimos[5].append(L"  X ");
    tetronimos[5].append(L"    ");

    tetronimos[6].append(L" XX ");
    tetronimos[6].append(L" X  ");
    tetronimos[6].append(L" X  ");
    tetronimos[6].append(L"    ");

    // Setup character attributes
    WORD attributes[9] =
    {
        FOREGROUND_BLUE,
        BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
        BACKGROUND_RED,
        BACKGROUND_GREEN,
        BACKGROUND_BLUE | BACKGROUND_INTENSITY,
        BACKGROUND_RED | BACKGROUND_GREEN,
        BACKGROUND_RED | BACKGROUND_BLUE,
        BACKGROUND_GREEN | BACKGROUND_BLUE,
        BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY
    };
    
    wchar_t glyphs[9] =
    {
        L'\u2592',
        L'\u25cb',
        L'\u25cf',
        L'\u2665',
        L'\u25a0',
        L'\u263c',
        L'\u2666',
        L'\u25ca',
        L'\u2660'
    };

    // Clear the screen
    for (int y = 0; y < field_height; ++y)
    {
        for (int x = 0; x < field_width; ++x)
        {
            screen_buffer[x + field_x_offset + (y + field_y_offset) * screen_width].Char.UnicodeChar = L' ';
            screen_buffer[x + field_x_offset + (y + field_y_offset) * screen_width].Attributes = 0;
        }
    }

    // Draw fixed elements
    for (int y = 0; y < field_height; ++y)
    {
        screen_buffer[field_x_offset - 1 + (y + field_y_offset) * screen_width].Attributes = BACKGROUND_BLUE;
        screen_buffer[field_x_offset + field_width + (y + field_y_offset) * screen_width].Attributes = BACKGROUND_BLUE;
    }

    for (int x = 0; x < field_width + 2; ++x)
    {
        screen_buffer[field_x_offset - 1 + x + (field_height + field_y_offset) * screen_width].Attributes = BACKGROUND_BLUE;
    }

    // Run the game
    bool exit = false;

    int current_piece = -1;
    int cpx = field_width / 2;
    int cpy = 0;
    int rotation = 0;
    int rflags = 0;
    int force_down_timer = force_down_time;
    vector<int> completed_lines;
    bool drop = false;

    random_device r;
    default_random_engine rg(r());
    uniform_int_distribution<int> uniform_dist(0, 6);

    while (!exit)
    {
        this_thread::sleep_for(chrono::milliseconds(update_delay));

        if (!completed_lines.empty())
        {
            attributes[(int)FieldElement::CompleteLine] ^= BACKGROUND_INTENSITY;

            if (force_down_timer > update_delay)
            {
                force_down_timer -= update_delay;
            }
            else
            {
                for (int line : completed_lines)
                {
                    for (int x = 0; x < field_width; ++x)
                    {
                        field[x + line * field_width] = FieldElement::Empty;
                    }
                }

                completed_lines.clear();

                drop = true;
            }
        }
        else if (drop)
        {
            // Find the lowest line into which the line above may drop
            int start_y = -1;

            for (int y = field_height; y >= 0; --y)
            {
                if (can_drop(y))
                {
                    start_y = y;
                    break;
                }
            }

            if (start_y >= 0)
            {
                drop = drop_lines(start_y);
            }
            else
            {
                // Look for any completed lines
                find_full_lines(0, field_height, completed_lines);

                for (int line : completed_lines)
                {
                    for (int x = 0; x < field_width; ++x)
                    {
                        field[x + line * field_width] = FieldElement::CompleteLine;
                    }
                }

                if (!completed_lines.empty())
                {
                    attributes[(int)FieldElement::CompleteLine] |= BACKGROUND_INTENSITY;
                    force_down_time = 500;
                }
            }
        }
        else if (current_piece < 0)
        {
            current_piece = uniform_dist(rg);
            cpx = field_width / 2;
            cpy = 0;
            rotation = 0;
            rflags &= ~3;
            force_down_timer = force_down_time;

            if (!valid_position(current_piece, cpx, cpy, rotation))
            {
                // Game over man
                exit = true;
                current_piece = -1;
            }
        }
        else
        {
            // Update
            if (key_down(L'A'))
            {
                if ((rflags & 1) == 0)
                {
                    int newr = rotation - 1;

                    if (newr < 0)
                    {
                        newr = 3;
                    }

                    if (valid_position(current_piece, cpx, cpy, newr))
                    {
                        rotation = newr;
                    }

                    rflags |= 1;
                }
            }
            else
            {
                rflags &= ~1;
            }
    
            if (key_down(L'D'))
            {
                if ((rflags & 2) == 0)
                {
                    int newr = rotation + 1;

                    if (newr > 3)
                    {
                        newr = 0;
                    }

                    if (valid_position(current_piece, cpx, cpy, newr))
                    {
                        rotation = newr;
                    }

                    rflags |= 2;
                }
            }
            else
            {
                rflags &= ~2;
            }

            if (key_down(VK_LEFT) && valid_position(current_piece, cpx - 1, cpy, rotation))
            {
                cpx--;
            }

            if (key_down(VK_RIGHT) && valid_position(current_piece, cpx + 1, cpy, rotation))
            {
                cpx++;
            }

            bool move_down = false;

            if (key_down(VK_DOWN))
            {
                move_down = true;
            }

            if (force_down_timer > update_delay)
            {
                force_down_timer -= update_delay;
            }
            else
            {
                move_down = true;
                force_down_timer = force_down_time;
            }

            if (move_down)
            {
                if (valid_position(current_piece, cpx, cpy + 1, rotation))
                {
                    cpy++;
                }
                else
                {
                    // Place the tetronimo in the field
                    for (int py = 0; py < 4; ++py)
                    {
                        for (int px = 0; px < 4; ++px)
                        {
                            if (tetronimos[current_piece][rotate(px, py, rotation)] == L'X')
                            {
                                field[cpx + px + (cpy + py) * field_width] = (FieldElement)((int)FieldElement::Tetronimo_0 + current_piece);
                            }
                        }
                    }

                    current_piece = -1;

                    // Look for any completed lines
                    find_full_lines(cpy, cpy + 4, completed_lines);

                    for (int line : completed_lines)
                    {
                        for (int x = 0; x < field_width; ++x)
                        {
                            field[x + line * field_width] = FieldElement::CompleteLine;
                        }
                    }

                    if (!completed_lines.empty())
                    {
                        attributes[(int)FieldElement::CompleteLine] |= BACKGROUND_INTENSITY;
                        force_down_time = 500;
                    }
                }
            }
        }

        // Draw
        for (int y = 0; y < field_height; ++y)
        {
            for (int x = 0; x < field_width; ++x)
            {
                screen_buffer[x + field_x_offset + (y + field_y_offset) * screen_width].Char.UnicodeChar = glyphs[(int)field[x + y * field_width]];
                screen_buffer[x + field_x_offset + (y + field_y_offset) * screen_width].Attributes = attributes[(int)field[x + y * field_width]];
            }
        }

        if (current_piece != -1)
        {
            FieldElement element = (FieldElement)((int)FieldElement::Tetronimo_0 + current_piece);

            for (int y = 0; y < 4; ++y)
            {
                for (int x = 0; x < 4; ++x)
                {
                    if (tetronimos[current_piece][rotate(x, y, rotation)] == L'X')
                    {
                        screen_buffer[x + field_x_offset + cpx + (y + field_y_offset + cpy) * screen_width].Char.UnicodeChar = glyphs[(int)element];
                        screen_buffer[x + field_x_offset + cpx + (y + field_y_offset + cpy) * screen_width].Attributes = attributes[(int)element];
                    }
                }
            }
        }

        COORD zero = { 0, 0 };
        SMALL_RECT write_region = { 0, 0, (SHORT)screen_width - 1, (SHORT)screen_height - 1 };
        WriteConsoleOutput(console, screen_buffer, buffer_size, zero, &write_region);
    }

    bool quit = false;

    // Draw a game over badge over the screen

    // Draw "Press Escape" at the bottom of the screen

    while (!quit)
    {
        if (key_down(VK_ESCAPE))
        {
            quit = true;
        }
    }

    return EXIT_SUCCESS;
}