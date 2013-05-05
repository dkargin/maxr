
#ifndef _FX_EFFECTS_H
#define _FX_EFFECTS_H

#include "clist.h"

class cGameGUI;

class cFx
{
protected:
	cFx (bool bottom_, int x, int y);

	int posX;
	int posY;
	int tick;
	int length;

public:
	virtual ~cFx ();

	const bool bottom;

	virtual bool isFinished () const;
	int getLength () const;

	virtual void draw (const cGameGUI& gameGUI) const = 0;
	virtual void playSound (const cGameGUI& gameGUI) const;
	virtual void run ();
};

class cFxMuzzle : public cFx
{
protected:
	cFxMuzzle (int x, int y, int dir_);
	void draw (const cGameGUI& gameGUI) const;

	SDL_Surface** image;
	int dir;
};

class cFxMuzzleBig : public cFxMuzzle
{
public:
	cFxMuzzleBig(int x, int y, int dir_);
};

class cFxMuzzleMed : public cFxMuzzle
{
public:
	cFxMuzzleMed(int x, int y, int dir_);
};

class cFxMuzzleMedLong : public cFxMuzzle
{
public:
	cFxMuzzleMedLong(int x, int y, int dir_);
};

class cFxMuzzleSmall : public cFxMuzzle
{
public:
	cFxMuzzleSmall(int x, int y, int dir_);
};

class cFxExplo : public cFx
{
protected:
	cFxExplo (int x, int y, int frames_);
	void draw (const cGameGUI& gameGUI) const;

	SDL_Surface** image;
	const int frames; //TODO: frames could be calculated (frames = w / h), if the width and height
	                  //of the grapics for one frame would be equal (which they aren't yet).
};

class cFxExploSmall : public cFxExplo
{
public:
	cFxExploSmall(int x, int y); //x, y is the center of the explosion
	void playSound (const cGameGUI& gameGUI) const;
};

class cFxExploBig : public cFxExplo
{
public:
	cFxExploBig(int x, int y);
	void playSound (const cGameGUI& gameGUI) const;
};

class cFxExploAir : public cFxExplo
{
public:
	cFxExploAir(int x, int y);
	void playSound (const cGameGUI& gameGUI) const;
};

class cFxExploWater : public cFxExplo
{
public:
	cFxExploWater(int x, int y);
	void playSound (const cGameGUI& gameGUI) const;
};

class cFxHit : public cFxExplo
{
public:
	cFxHit(int x, int y);
	void playSound (const cGameGUI& gameGUI) const;
};

class cFxAbsorb : public cFxExplo
{
public:
	cFxAbsorb(int x, int y);
	void playSound (const cGameGUI& gameGUI) const;
};

class cFxFade : public cFx
{
protected:
	cFxFade (int x, int y, bool bottom, int start, int end);
	void draw (const cGameGUI& gameGUI) const;

	SDL_Surface** image;
	const int alphaStart;
	const int alphaEnd;
};


class cFxSmoke : public cFxFade
{
public:
	cFxSmoke(int x, int y, bool bottom); //x, y is the center of the effect
};

class cFxCorpse : public cFxFade
{
public:
	cFxCorpse(int x, int y);
};

class cFxTracks : public cFx
{
private:
	SDL_Surface** image;
	const int alphaStart;
	const int alphaEnd;
	const int dir;
public:
	cFxTracks(int x, int y, int dir_);
	void draw (const cGameGUI& gameGUI) const;
};


class cFxRocket : public cFx
{
private:
	const int speed;
	cList<cFx*> subEffects;
	SDL_Surface** image;
	int dir;
	int distance;
	const int startX;
	const int startY;
	const int endX;
	const int endY;
public:
	cFxRocket (int startX_, int startY_, int endX_, int endY_, int dir_, bool bottom);
	~cFxRocket ();
	void draw (const cGameGUI& gameGUI) const;
	void run ();
	bool isFinished () const; //return true, when the last smoke effect is finished.
	                          //getLength() returns only the time until the rocket has reached the destiantion
};

class cFxDarkSmoke : public cFx
{
private:
	float dx;
	float dy;
	const int alphaStart;
	const int alphaEnd;
	const int frames;
	SDL_Surface** image;
public:
	cFxDarkSmoke (int x, int y, int alpha, float windDir);
	void draw (const cGameGUI& gameGUI) const;
};


#endif
