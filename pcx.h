//////////////////////////////////////////////////////////////////////////////
// M.A.X. - pcx.h
//////////////////////////////////////////////////////////////////////////////
#ifndef pcxH
#define pcxH
#include "defines.h"
#include "SDL.h"

// Prototypen ////////////////////////////////////////////////////////////////
SDL_Surface *LoadPCX(char *name,bool NoHW=false);
void LoadPCXtoSF(char *name,SDL_Surface *sf);

#endif
