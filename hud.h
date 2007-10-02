//////////////////////////////////////////////////////////////////////////////
// M.A.X. - hud.h
//////////////////////////////////////////////////////////////////////////////
#ifndef hudH
#define hudH
#include "defines.h"
#include "SDL.h"

// Die Hud-Klasse ////////////////////////////////////////////////////////////
class cHud{
public:
  cHud();
  ~cHud();

  bool TNT,Radar,Nebel,Gitter,Scan,Reichweite,Munition,Treffer,Farben,Status,Studie,Lock;
  bool Praeferenzen;
  bool PlayFLC,PausePressed,PlayPressed;
  bool HelpPressed,ChatPressed,EndePressed;
  bool ErledigenPressed,NextPressed,PrevPressed;
  bool CenterPressed,DateiPressed,LogPressed;
  int Zoom,LastZoom;
  int OffX,OffY;
  bool Ende;
  bool LastOverEnde;

  void SwitchTNT(bool set);
  void SwitchRadar(bool set);
  void SwitchNebel(bool set);
  void SwitchGitter(bool set);
  void SwitchScan(bool set);
  void SwitchReichweite(bool set);
  void SwitchMunition(bool set);
  void SwitchTreffer(bool set);
  void SwitchFarben(bool set);
  void SwitchStatus(bool set);
  void SwitchStudie(bool set);
  void SwitchLock(bool set);
  void DoZoom(int x,int y=274);
  void SetZoom(int zoom,int DestY=274);
  void DoScroll(int dir);
  void DoMinimapClick(int x,int y);
  void PraeferenzenButton(bool set);
  void PauseButton(bool set);
  void PlayButton(bool set);
  void HelpButton(bool set);
  void ChatButton(bool set);
  void LogButton(bool set);  
  void EndeButton(bool set);
  void ErledigenButton(bool set);
  void NextButton(bool set);
  void PrevButton(bool set);
  void CenterButton(bool set);
  void DateiButton(bool set);
  void ScaleSurfaces(void);    

  void CheckButtons(void);
  void CheckOneClick(void);
  void DoAllHud(void);
  void CheckScroll(bool pure=false);
  void ChechMouseOver(void);
  void ResetVideoMonitor(void);
  void ShowRunde(void);
  void MakeMeMyEnd(void);
};

#endif
