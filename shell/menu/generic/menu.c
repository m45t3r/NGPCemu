#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>

#include "scaler.h"
#include "font_drawing.h"
#include "sound_output.h"
#include "video_blit.h"
#include "config.h"
#include "menu.h"

#ifdef BITTBOY
#define CONFIRM_BUTTON SDLK_LALT
#define BACK_BUTTON SDLK_LCTRL
#else
#define CONFIRM_BUTTON SDLK_LCTRL
#define BACK_BUTTON SDLK_LALT
#endif

extern SDL_Surface *sdl_screen;
extern char GameName_emu[512];

extern void SaveState(const char* path, uint_fast8_t load);

t_config option;

static char home_path[256], save_path[256], eeprom_path[256], conf_path[256];
static uint32_t controls_chosen = 0;

char EEPROM_filepath[512];

static uint8_t save_slot = 0;

#ifdef IPU_SCALE
static const int8_t upscalers_available = 1
#else
static const int8_t upscalers_available = 2
#endif
#ifdef SCALE2X_UPSCALER
+1
#endif
;

static void SaveState_Menu(uint_fast8_t load_mode, uint_fast8_t slot)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s_%d.sts", save_path, GameName_emu, slot);
	SaveState(tmp,load_mode);
}

static void EEPROM_Menu(uint_fast8_t load_mode)
{
	snprintf(EEPROM_filepath, sizeof(EEPROM_filepath), "%s/%s.sav", eeprom_path, GameName_emu);
}

static void config_load()
{
	char config_path[512];
	FILE* fp;
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);

	fp = fopen(config_path, "rb");
	if (fp)
	{
		fread(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
	else
	{
		/* Default mapping for Horizontal */
		option.config_buttons[0] = 273;
		option.config_buttons[1] = 274;
		option.config_buttons[2] = 276;
		option.config_buttons[3] = 275;
#ifdef BITTBOY
		option.config_buttons[4] = 308;
		option.config_buttons[5] = 306;
		option.config_buttons[6] = 13;
#else
		option.config_buttons[4] = 306;
		option.config_buttons[5] = 308;
		option.config_buttons[6] = 13;
#endif
		option.fullscreen = 1;
	}
}

static void config_save()
{
	FILE* fp;
	char config_path[512];
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);
	
	fp = fopen(config_path, "wb");
	if (fp)
	{
		fwrite(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
}

static const char* Return_Text_Button(uint32_t button)
{
	switch(button)
	{
		/* UP button */
		case 273:
			return "DPAD UP";
		break;
		/* DOWN button */
		case 274:
			return "DPAD DOWN";
		break;
		/* LEFT button */
		case 276:
			return "DPAD LEFT";
		break;
		/* RIGHT button */
		case 275:
			return "DPAD RIGHT";
		break;
#ifdef BITTBOY
		/* A button */
		case 308:
			return "A button";
		break;
		/* B button */
		case 306:
			return "B button";
#else
		/* A button */
		case 306:
			return "A button";
		break;
		/* B button */
		case 308:
			return "B button";
#endif
		break;
		/* X button */
		case 304:
			return "X button";
		break;
		/* Y button */
		case 32:
			return "Y button";
		break;
		/* L button */
		case 9:
			return "L button";
		break;
		/* R button */
		case 8:
			return "R button";
		break;
		/* Power button */
		case 279:
			return "L2 button";
		break;
		/* Brightness */
		case 51:
			return "R2 button";
		break;
		/* Volume - */
		case 38:
			return "Volume -";
		break;
		/* Volume + */
		case 233:
			return "Volume +";
		break;
		/* Start */
		case 13:
			return "Start button";
		break;
		/* Select */
		case 1:
			return "Select button";
		break;
		default:
			return "...";
		break;
	}	
}

static void Input_Remapping()
{
	SDL_Event Event;
	char text[50];
	uint32_t pressed = 0;
	int32_t currentselection = 1;
	int32_t exit_input = 0;
	uint32_t exit_map = 0;
	
	while(!exit_input)
	{
		pressed = 0;
		SDL_FillRect( backbuffer, NULL, 0 );
		
        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection < 1)
                        {
							if (currentselection > 9) currentselection = 12;
							else currentselection = 9;
						}
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 10)
                        {
							currentselection = 1;
						}
                        break;
                    case CONFIRM_BUTTON:
                    case SDLK_RETURN:
                        pressed = 1;
					break;
                    case SDLK_ESCAPE:
                        option.config_buttons[currentselection - 1] = 0;
					break;
                    case BACK_BUTTON:
                        exit_input = 1;
					break;
                    case SDLK_BACKSPACE:
						controls_chosen = 1;
					break;
                    case SDLK_TAB:
						controls_chosen = 0;
					break;
					default:
					break;
                }
            }
        }

        if (pressed)
        {
			SDL_Delay(160);
            switch(currentselection)
            {
                default:
					SDL_FillRect( backbuffer, NULL, 0 );
					print_string("Please press button for mapping", TextWhite, TextBlue, 37, 108, backbuffer->pixels);
					bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,backbuffer->w,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
					SDL_Flip(sdl_screen);
					exit_map = 0;
					while( !exit_map )
					{
						while (SDL_PollEvent(&Event))
						{
							if (Event.type == SDL_KEYDOWN)
							{
								if (Event.key.keysym.sym != SDLK_RCTRL)
								{
									option.config_buttons[currentselection - 1] = Event.key.keysym.sym;
									exit_map = 1;
								}
							}
						}
					}
				break;
            }
        }
        
        if (currentselection > 7) currentselection = 7;
		
		print_string("Press [A] to map to a button", TextWhite, TextBlue, 50, 210, backbuffer->pixels);
		print_string("Press [B] to Exit", TextWhite, TextBlue, 85, 225, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "DPAD UP   : %s\n", Return_Text_Button(option.config_buttons[0]));
		if (currentselection == 1) print_string(text, TextRed, 0, 5, 25+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 25+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "DPAD DOWN : %s\n", Return_Text_Button(option.config_buttons[1]));
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 45+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 45+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "DPAD LEFT : %s\n", Return_Text_Button(option.config_buttons[2]));
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 65+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "DPAD RIGHT: %s\n", Return_Text_Button(option.config_buttons[3]));
		if (currentselection == 4) print_string(text, TextRed, 0, 5, 85+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "A BUTTON  : %s\n", Return_Text_Button(option.config_buttons[4]));
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 105+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 105+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "B BUTTON  : %s\n", Return_Text_Button(option.config_buttons[5]));
		if (currentselection == 6) print_string(text, TextRed, 0, 5, 125+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "OPTION   : %s\n", Return_Text_Button(option.config_buttons[6]));
		if (currentselection == 7) print_string(text, TextRed, 0, 5, 145+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 145+2, backbuffer->pixels);

		bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,backbuffer->w,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
		SDL_Flip(sdl_screen);
	}
	
	config_save();
}

void Menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    SDL_Event Event;
    
    Set_Video_Menu();
    
	/* Save eeprom settings each time we bring up the menu */
	EEPROM_Menu(0);
    
    while (((currentselection != 1) && (currentselection != 6)) || (!pressed))
    {
        pressed = 0;
        
        SDL_FillRect( backbuffer, NULL, 0 );

		print_string("NGPCEmu - Date: " __DATE__, TextWhite, 0, 5, 15, backbuffer->pixels);
		
		if (currentselection == 1) print_string("Continue", TextRed, 0, 5, 45, backbuffer->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 45, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", save_slot);
		
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 65, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Save State %d", save_slot);
		
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 85, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85, backbuffer->pixels);
		
        if (currentselection == 4)
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Stretched", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Keep scaled", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Native", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }
        else
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Stretched", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Keep scaled", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Native", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }

		if (currentselection == 5) print_string("Input remapping", TextRed, 0, 5, 125, backbuffer->pixels);
		else print_string("Input remapping", TextWhite, 0, 5, 125, backbuffer->pixels);
		
		if (currentselection == 6) print_string("Quit", TextRed, 0, 5, 145, backbuffer->pixels);
		else print_string("Quit", TextWhite, 0, 5, 145, backbuffer->pixels);

		print_string("Mednafen Fork by gameblabla", TextWhite, 0, 5, 205, backbuffer->pixels);
		print_string("Credits: Ryphecha, libretro", TextWhite, 0, 5, 225, backbuffer->pixels);

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = 6;
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 7)
                            currentselection = 1;
                        break;
                    case SDLK_END:
                    case SDLK_RCTRL:
		    case BACK_BUTTON:
						pressed = 1;
						currentselection = 1;
						break;
                    case CONFIRM_BUTTON:
                    case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (save_slot > 0) save_slot--;
							break;
                            case 4:
							option.fullscreen--;
							if (option.fullscreen < 0)
								option.fullscreen = upscalers_available;
							break;
                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                save_slot++;
								if (save_slot == 10)
									save_slot = 9;
							break;
                            case 4:
                                option.fullscreen++;
                                if (option.fullscreen > upscalers_available)
                                    option.fullscreen = 0;
							break;
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = 7;
				pressed = 1;
			}
        }

        if (pressed)
        {
            switch(currentselection)
            {
				case 5:
					Input_Remapping();
				break;
                case 4 :
                    option.fullscreen++;
                    if (option.fullscreen > upscalers_available)
                        option.fullscreen = 0;
                    break;
                case 2 :
                    SaveState_Menu(1, save_slot);
					currentselection = 1;
                    break;
                case 3 :
					SaveState_Menu(0, save_slot);
					currentselection = 1;
				break;
				default:
				break;
            }
        }

		bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,backbuffer->w,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
		SDL_Flip(sdl_screen);
    }
    
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #ifdef SDL_TRIPLEBUF
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #endif
    
    if (currentselection == 6)
    {
        done = 1;
	}
	else
	{
		SDL_Delay(160);
	}
	
	/* Switch back to emulator core */
	emulator_state = 0;
	Set_Video_InGame();
}

void Init_Configuration()
{
	snprintf(home_path, sizeof(home_path), "%s/.ngpcemu", getenv("HOME"));
	
	snprintf(conf_path, sizeof(conf_path), "%s/conf", home_path);
	snprintf(save_path, sizeof(save_path), "%s/sstates", home_path);
	snprintf(eeprom_path, sizeof(eeprom_path), "%s/eeprom", home_path);
	
	/* We check first if folder does not exist. 
	 * Let's only try to create it if so in order to decrease boot times.
	 * */
	
	if (access( home_path, F_OK ) == -1)
	{ 
		mkdir(home_path, 0755);
	}
	
	if (access( save_path, F_OK ) == -1)
	{
		mkdir(save_path, 0755);
	}
	
	if (access( conf_path, F_OK ) == -1)
	{
		mkdir(conf_path, 0755);
	}
	
	if (access( eeprom_path, F_OK ) == -1)
	{
		mkdir(eeprom_path, 0755);
	}
	
	/* Load eeprom file if it exists */
	EEPROM_Menu(1);
	
	config_load();
}
