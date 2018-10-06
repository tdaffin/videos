/*
OneLoneCoder.com - Code-It-Yourself! Worms Part #3
"stuuup-iid...." - @Javidx9

Disclaimer
~~~~~~~~~~
I don't care what you use this for. It's intended to be educational, and perhaps
to the oddly minded - a little bit of fun. Please hack this, change it and use it
in any way you see fit. BUT, you acknowledge that I am not responsible for anything
bad that happens as a result of your actions. However, if good stuff happens, I
would appreciate a shout out, or at least give the blog some publicity for me.
Cheers!

Background
~~~~~~~~~~
Worms is a classic game where several teams of worms use a variety of weaponry
to elimiate each other from a randomly generated terrain.

This code is the third part of a series that show how to make your own Worms game
from scratch in C++!

Controls A = Aim Left, S = Aim Right, Z = Jump, Space = Charge Weapon, TAB = Zoom

Author
~~~~~~
Twitter: @javidx9
Blog: www.onelonecoder.com

Video:
~~~~~~
Part #1 https://youtu.be/EHlaJvQpW3U
Part #2 https://youtu.be/pV2qYJjCdxM
Part #3 https://youtu.be/NKK5tIRZqyQ

Last Updated: 19/12/2017
*/

#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

#include "olcConsoleGameEngineSDL.h"


class cMap {
public:
	virtual char GetMap(int x, int y) = 0;
};

class cPhysicsObject
{
public:
	cPhysicsObject(float x = 0.0f, float y = 0.0f)
	{
		px = x;
		py = y;
	}

public:
	float px = 0.0f;				// Position
	float py = 0.0f;
	float vx = 0.0f;				// Velocity
	float vy = 0.0f;
	float ax = 0.0f;				// Acceleration
	float ay = 0.0f;
	float radius = 4.0f;			// Bounding rectangle for collisions
	float fFriction = 0.0f;			// Actually, a dampening factor is a more accurate name
	int nBounceBeforeDeath = -1;	// How many time object can bounce before death
	bool bDieWhenStable = false;	// Indicate if object should die when it becomes stable
	bool bDead = false;				// Flag to indicate object should be removed
	bool bStable = false;			// Has object stopped moving

	// Make class abstract
	virtual void Draw(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY, bool bPixel = false) = 0;
	virtual int BounceDeathAction() = 0;
	virtual bool Damage(float d) = 0;

	// Optional overtides
	virtual int StableDeathAction() { return 0; }

	virtual bool CheckCollision(cMap* map, float fPotentialX, float fPotentialY, float& fResponseX, float& fResponseY)
	{
		fResponseX = 0;
		fResponseY = 0;
		bool bCollision = false;

		// Iterate through semicircle of objects radius rotated to direction of travel
		float fAngle = atan2f(vy, vx);
		for (float r = fAngle - 3.14159f / 2.0f; r < fAngle + 3.14159f / 2.0f; r += 3.14159f / 8.0f)
		{
			// Calculate test point on circumference of circle
			float fTestPosX = radius * cosf(r) + fPotentialX;
			float fTestPosY = radius * sinf(r) + fPotentialY;

			// Test if any points on semicircle intersect with terrain
			if (map->GetMap(fTestPosX, fTestPosY) > 0)
			{
				// Accumulate collision points to give an escape response vector
				// Effectively, normal to the areas of contact
				fResponseX += fPotentialX - fTestPosX;
				fResponseY += fPotentialY - fTestPosY;
				bCollision = true;
			}
		}
		return bCollision;
	}

	virtual bool CheckStability(bool bCollision, float fResponseX, float fResponseY, float fMagResponse, float fMagVelocity) {
		// Turn off movement when tiny
		return fMagVelocity < 0.1f;
	}
};


class cHealthObject : public cPhysicsObject {
public:
	cHealthObject(float x, float y)	: cPhysicsObject(x, y)
	{

	}

	void DrawHealth(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY)
	{
		// Draw health bar for
		for (int i = 0; i < 11 * fHealth; i++)
		{
			engine->Draw(px - 5 + i - fOffsetX, py - fOffsetY, PIXEL_SOLID, FG_BLUE);
			engine->Draw(px - 5 + i - fOffsetX, py + 1 - fOffsetY, PIXEL_SOLID, FG_BLUE);
		}
	}

public:
	float fHealth = 1.0f;

};


class cDummy : public cHealthObject // Does nothing, shows a marker that helps with physics debug and test
{
public:
	cDummy(float x = 0.0f, float y = 0.0f) : cHealthObject(x, y)
	{
		//radius = 0.5f;
		fFriction = 0.4f;
		vx = 0;
		vy = 0;
		bDead = false;
		nBounceBeforeDeath = -1;
		bDieWhenStable = false;
		bStable = false;
	}

	virtual void Draw(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_WHITE);
		DrawHealth(engine, fOffsetX, fOffsetY + 6);
	}

	virtual int BounceDeathAction()
	{
		return 40; // Huge explosion!
	}

	virtual bool Damage(float d) // Reduce health by said amount
	{
		fHealth -= d;
		if (fHealth <= 0)
		{	// Dead!
			fHealth = 0.0f;
			bDieWhenStable = true;
			//nBounceBeforeDeath = -1;
		}
		return fHealth > 0;
	}

	virtual int StableDeathAction() {
		return -40; // Huge explosion!
	}

	virtual bool CheckStability(bool bCollision, float fResponseX, float fResponseY, float fMagResponse, float fMagVelocity) {
		// Turn off movement when tiny
		//return fMagVelocity < 0.1f;

		// Turn off movement when tiny and in collision with mostly horizontal
		if (bCollision && fMagVelocity < 0.1f) {
			if (-fResponseY/fMagResponse > 0.9f)
				return true;
		}
		return false;
	}

private:
	static vector<pair<float, float>> vecModel;
};

vector<pair<float, float>> DefineDummy()
{
	// Defines a circle with a line fom center to edge
	vector<pair<float, float>> vecModel;
	vecModel.push_back({ 0.0f, 0.0f });
	for (int i = 0; i < 10; i++)
		vecModel.push_back({ cosf(i / 9.0f * 2.0f * 3.14159f) , sinf(i / 9.0f * 2.0f * 3.14159f) });
	return vecModel;
}
vector<pair<float, float>> cDummy::vecModel = DefineDummy();


class cDebris : public cPhysicsObject // a small rock that bounces
{
public:
	cDebris(float x = 0.0f, float y = 0.0f) : cPhysicsObject(x, y)
	{
		// Set velocity to random direction and size for "boom" effect
		vx = 10.0f * cosf(((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f);
		vy = 10.0f * sinf(((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f);
		radius = 1.0f;
		fFriction = 0.8f;
		bDead = false;
		bStable = false;
		nBounceBeforeDeath = 2; // After 2 bounces, dispose
	}

	virtual void Draw(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_DARK_GREEN);
	}

	virtual int BounceDeathAction()
	{
		return -2; // Build a circle
	}

	virtual bool Damage(float d)
	{
		return true; // Cannot be damaged
	}

private:
	static vector<pair<float, float>> vecModel;

};

vector<pair<float, float>> DefineDebris()
{
	// A small unit rectangle
	vector<pair<float, float>> vecModel;
	vecModel.push_back({ 0.0f, 0.0f });
	vecModel.push_back({ 1.0f, 0.0f });
	vecModel.push_back({ 1.0f, 1.0f });
	vecModel.push_back({ 0.0f, 1.0f });
	return vecModel;
}

vector<pair<float, float>> cDebris::vecModel = DefineDebris();


const int nbrIdx[3][3] = {
	{3, 2, 1},
	{4,-1, 0},
	{5, 6, 7}};
const int nbrs[][2] = {{1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}, {0,1}, {1,1}};

class cPixel : public cPhysicsObject
{
public:
	cPixel(float x, float y): cPhysicsObject(x, y)
	{
		radius = 0.5f;
		fFriction = 0.1f;
		vx = 0;
		vy = 0;
		bDead = false;
		nBounceBeforeDeath = 1;
		bDieWhenStable = true;
		bStable = false;
	}

	virtual void Draw(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		engine->Draw(px - fOffsetX, py - fOffsetY);
	}

	virtual int BounceDeathAction()
	{
		return 0; // Nothing, just fade
	}

	virtual bool Damage(float d){
		return true; // Cannot be damaged
	}

	virtual int StableDeathAction()
	{
		return 1;
	}

	virtual bool CheckCollision(cMap* map, float fPotentialX, float fPotentialY, float& fResponseX, float& fResponseY)
	{
		fResponseX = 0;
		fResponseY = 0;
		bool bCollision = false;

		int dx = vx > 0 ? 1 : (vx < 0 ? -1 : 0);
		int dy = vy > 0 ? 1 : (vy < 0 ? -1 : 0);

		int x = (int)fPotentialX;
		int y = (int)fPotentialY;
		if (map->GetMap(x, y) > 0){
			// Estimate an escape response vector effectively, normal to the areas of contact
			int cx = (int)px; 
			int cy = (int)py;
			if (cx == x){
				fResponseX = 0;
				fResponseY = -dy;//px - fPotentialX;
			} else if (cy == y){
				fResponseX = -dx;
				fResponseY = 0;
			} else if (cx == x && cy == y){
				fResponseX = 0;
				fResponseY = -1; // up!
			} else {
				fResponseX = -dx;
				fResponseY = -dy;
			}

			//fResponseX += fPotentialX - x;
			//fResponseY += fPotentialY - y;
			bCollision = true;
		}

		// Check up to 3 of eight neighbors in direction of travel
		/*
		int i0 = nbrIdx[dx+1][dy+1];
		int idxs[3] = {i0, (i0+1)%8, (i0+7)%8};
		for (int i=0; i<3; ++i){
			float fTestPosX = radius * cosf(r) + fPotentialX;
			float fTestPosY = radius * sinf(r) + fPotentialY;
		}
		for (float r = fAngle - 3.14159f / 2.0f; r < fAngle + 3.14159f / 2.0f; r += 3.14159f / 8.0f)
		{
			// Calculate test point on circumference of circle
			float fTestPosX = radius * cosf(r) + fPotentialX;
			float fTestPosY = radius * sinf(r) + fPotentialY;

			// Test if any points on semicircle intersect with terrain
			if (map->GetMap(fTestPosX, fTestPosY) != 0)
			{
				// Accumulate collision points to give an escape response vector
				// Effectively, normal to the areas of contact
				fResponseX += fPotentialX - fTestPosX;
				fResponseY += fPotentialY - fTestPosY;
				bCollision = true;
			}
		}*/
		return bCollision;
	}

	virtual bool CheckStability(bool bCollision, float fResponseX, float fResponseY, float fMagResponse, float fMagVelocity) {
		// Turn off movement when tiny
		//return fMagVelocity < 0.1f;

		// Turn off movement when tiny and in collision with mostly horizontal
		if (bCollision && fMagVelocity < 0.1f) {
			if (-fResponseY/fMagResponse > 0.9f)
				return true;
		}
		return false;
	}
};


class cMissile : public cPhysicsObject // A projectile weapon
{
public:
	cMissile(float x = 0.0f, float y = 0.0f, float _vx = 0.0f, float _vy = 0.0f) : cPhysicsObject(x, y)
	{
		radius = 2.5f;
		fFriction = 0.5f;
		vx = _vx;
		vy = _vy;
		bDead = false;
		nBounceBeforeDeath = 1;
		bStable = false;
	}

	virtual void Draw(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_BLACK);
	}

	virtual int BounceDeathAction()
	{
		return 20; // Explode Big
	}

	virtual bool Damage(float d)
	{
		return true;
	}

private:
	static vector<pair<float, float>> vecModel;
};

vector<pair<float, float>> DefineMissile()
{
	// Defines a rocket like shape
	vector<pair<float, float>> vecModel;
	vecModel.push_back({ 0.0f, 0.0f });
	vecModel.push_back({ 1.0f, 1.0f });
	vecModel.push_back({ 2.0f, 1.0f });
	vecModel.push_back({ 2.5f, 0.0f });
	vecModel.push_back({ 2.0f, -1.0f });
	vecModel.push_back({ 1.0f, -1.0f });
	vecModel.push_back({ 0.0f, 0.0f });
	vecModel.push_back({ -1.0f, -1.0f });
	vecModel.push_back({ -2.5f, -1.0f });
	vecModel.push_back({ -2.0f, 0.0f });
	vecModel.push_back({ -2.5f, 1.0f });
	vecModel.push_back({ -1.0f, 1.0f });

	// Scale points to make shape unit(ish) sized
	for (auto &v : vecModel)
	{
		v.first /= 1.5f; v.second /= 1.5f;
	}
	return vecModel;
}

vector<pair<float, float>> cMissile::vecModel = DefineMissile();

class cWorm : public cHealthObject // A unit, or worm
{
public:
	cWorm(float x = 0.0f, float y = 0.0f) : cHealthObject(x, y)
	{
		radius = 3.5f;
		fFriction = 0.2f;
		bDead = false;
		nBounceBeforeDeath = -1;
		bStable = false;

		// load sprite data from sprite file
		if (sprWorm == nullptr)
			sprWorm = new olcSprite(L"./worms/worms1.spr");
	}

	virtual void Draw(olcConsoleGameEngine *engine, float fOffsetX, float fOffsetY, bool bPixel = false)
	{
		if (bIsPlayable) // Draw Worm Sprite with health bar, in team colours
		{
			engine->DrawPartialSprite(px - fOffsetX - radius, py - fOffsetY - radius, sprWorm, nTeam * 8, 0, 8, 8);
			
			// Draw health bar for worm
			DrawHealth(engine, fOffsetX, fOffsetY + 5);
		}
		else // Draw tombstone sprite for team colour
		{
			engine->DrawPartialSprite(px - fOffsetX - radius, py - fOffsetY - radius, sprWorm, nTeam * 8, 8, 8, 8);
		}
	}

	virtual int BounceDeathAction()
	{
		return 0; // Nothing
	}

	virtual bool Damage(float d) // Reduce worm's health by said amount
	{
		fHealth -= d;
		if (fHealth <= 0)
		{ // Worm has died, no longer playable
			fHealth = 0.0f;
			bIsPlayable = false;
		}
		return fHealth > 0;
	}

public:
	float fShootAngle = 0.0f;
	int nTeam = 0;	// ID of which team this worm belongs to
	bool bIsPlayable = true;

private:
	static olcSprite *sprWorm;
};

olcSprite* cWorm::sprWorm = nullptr;



class cTeam // Defines a group of worms
{
public:
	vector<cWorm*> vecMembers;
	int nCurrentMember = 0;		// Index into vector for current worms turn
	int nTeamSize = 0;			// Total number of worms in team

	bool IsTeamAlive()
	{	
		// Iterate through all team members, if any of them have >0 health, return true;
		bool bAllDead = false;
		for (auto w : vecMembers)
			bAllDead |= (w->fHealth > 0.0f);
		return bAllDead;
	}

	cWorm* GetNextMember()
	{
		// Return a pointer to the next team member that is valid for control
		do {
			nCurrentMember++;
			if (nCurrentMember >= nTeamSize) nCurrentMember = 0;
		} while (vecMembers[nCurrentMember]->fHealth <= 0);
		return vecMembers[nCurrentMember];
	}	
};


// Main Game Engine Class
class OneLoneCoder_Worms : public olcConsoleGameEngine, cMap // The game
{
public:
	OneLoneCoder_Worms()
	{
		m_sAppName = L"Worms";
	}

	char GetMap(int x, int y){
		// Constrain to test within map boundary
		/*if (x >= nMapWidth) return 1;//x = nMapWidth - 1;
		if (y >= nMapHeight) return 1;//y = nMapHeight - 1;
		if (x < 0) return 1;//x = 0;
		if (y < 0) y = 0; // Allows things to go off the top*/

		if (x >= nMapWidth) x = nMapWidth - 1;
		if (y >= nMapHeight) y = nMapHeight - 1;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		return map[y * nMapWidth + x];
	}

private:
	// Terrain size
	int nMapWidth = 1024;
	int nMapHeight = 512;
	char *map = nullptr;

	// Camera Coordinates
	float fCameraPosX = 0.0f;
	float fCameraPosY = 0.0f;
	float fCameraPosXTarget = 0.0f;
	float fCameraPosYTarget = 0.0f;

	// list of things that exist in game world
	list<unique_ptr<cPhysicsObject>> listObjects;

	cPhysicsObject* pObjectUnderControl = nullptr;		// Pointer to object currently under control
	cPhysicsObject* pCameraTrackingObject = nullptr;	// Pointer to object that camera should track

	// Flags that govern/are set by game state machine
	bool bCheat = false;					// Cheat mode -- stop the clock!
	bool bZoomOut = false;					// Render whole map
	bool bGameIsStable = false;				// All physics objects are stable
	bool bEnablePlayerControl = true;		// The player is in control, keyboard input enabled
	bool bEnableComputerControl = false;	// The AI is in control
	bool bEnergising = false;				// Weapon is charging
	bool bFireWeapon = false;				// Weapon should be discharged
	bool bShowCountDown = false;			// Display turn time counter on screen
	bool bPlayerHasFired = false;			// Weapon has been discharged

	float fEnergyLevel = 0.0f;				// Energy accumulated through charging (player only)
	float fTurnTime = 0.0f;					// Time left to take turn

	// Vector to store teams
	vector<cTeam> vecTeams;

	// Current team being controlled
	int nCurrentTeam = 0;

	// AI control flags
	bool bAI_Jump = false;				// AI has pressed "JUMP" key
	bool bAI_AimLeft = false;			// AI has pressed "AIM_LEFT" key
	bool bAI_AimRight = false;			// AI has pressed "AIM_RIGHT" key
	bool bAI_Energise = false;			// AI has pressed "FIRE" key

	
	float fAITargetAngle = 0.0f;		// Angle AI should aim for
	float fAITargetEnergy = 0.0f;		// Energy level AI should aim for
	float fAISafePosition = 0.0f;		// X-Coordinate considered safe for AI to move to
	cWorm* pAITargetWorm = nullptr;		// Pointer to worm AI has selected as target
	float fAITargetX = 0.0f;			// Coordinates of target missile location
	float fAITargetY = 0.0f;

	enum GAME_STATE
	{
		GS_RESET = 0,
		GS_GENERATE_TERRAIN = 1,
		GS_GENERATING_TERRAIN,		
		GS_ALLOCATE_UNITS,
		GS_ALLOCATING_UNITS,
		GS_START_PLAY,
		GS_CAMERA_MODE,
		GS_GAME_OVER1,
		GS_GAME_OVER2
	} nGameState, nNextState;


	enum AI_STATE
	{
		AI_ASSESS_ENVIRONMENT = 0,
		AI_MOVE,
		AI_CHOOSE_TARGET,
		AI_POSITION_FOR_TARGET,
		AI_AIM,
		AI_FIRE,
	} nAIState, nAINextState;

	virtual bool OnUserCreate()
	{
		// Create Map
		map = new  char[nMapWidth * nMapHeight];
		memset(map, 0, nMapWidth*nMapHeight * sizeof( char));
		
		// Set initial states for state machines
		nGameState = GS_RESET;
		nNextState = GS_RESET;
		nAIState = AI_ASSESS_ENVIRONMENT;
		nAINextState = AI_ASSESS_ENVIRONMENT;

		bGameIsStable = false;
		return true;
	}

	virtual bool OnUserUpdate(float fElapsedTime)
	{
		// Tab key toggles between whole map view and up close view
		if (m_keys[VK_TAB].bReleased)
			bZoomOut = !bZoomOut;

		if (m_keys[VK_ESCAPE].bReleased)
			bCheat = !bCheat;

		auto worldX = [&](int sx)
		{
			if (bZoomOut)
				return (float)sx/(float)ScreenWidth() * (float)nMapWidth;
			else
				return sx + fCameraPosX;
		};
		auto worldY = [&](int sy)
		{
			if (bZoomOut)
				return (float)sy/(float)ScreenHeight() * (float)nMapHeight;
			else
				return sy + fCameraPosY;
		};

		// Left click to cause small explosion
		if (m_mouse[0].bReleased)
			Boom(worldX(m_mousePosX), worldY(m_mousePosY), 10.0f);

		// Right click to drop missile
		if (m_mouse[1].bReleased)
			listObjects.push_back(unique_ptr<cMissile>(new cMissile(worldX(m_mousePosX), worldY(m_mousePosY))));

		// Middle click to spawn worm/unit
		if (m_mouse[2].bReleased){
			auto dummy = new cDummy(worldX(m_mousePosX), worldY(m_mousePosY));
			dummy->fFriction = 0.4f;//1.0f;
			listObjects.push_back(unique_ptr<cDummy>(dummy));
			//listObjects.push_back(unique_ptr<cWorm>(new cWorm(worldX(m_mousePosX), worldY(m_mousePosY))));
		}

		// Mouse Edge Map Scroll
		float fMapScrollSpeed = 400.0f;
		if (m_mousePosX < 5) fCameraPosX -= fMapScrollSpeed * fElapsedTime;
		if (m_mousePosX > ScreenWidth() - 5) fCameraPosX += fMapScrollSpeed * fElapsedTime;
		if (m_mousePosY < 5) fCameraPosY -= fMapScrollSpeed * fElapsedTime;
		if (m_mousePosY > ScreenHeight() - 5) fCameraPosY += fMapScrollSpeed * fElapsedTime;

		// Control Supervisor
		switch (nGameState)
		{
		case GS_RESET:
			{
				bEnablePlayerControl = false;
				bGameIsStable = false;
				bPlayerHasFired = false;
				bShowCountDown = false;			
				nNextState = GS_GENERATE_TERRAIN;
			}
			break;
		
		case GS_GENERATE_TERRAIN:
			{
				bZoomOut = true;
				CreateMap();
				bGameIsStable = false;
				bShowCountDown = false;
				nNextState = GS_GENERATING_TERRAIN;
			}
			break;

		case GS_GENERATING_TERRAIN:
			{
				bShowCountDown = false;
				if (bGameIsStable)
					nNextState = GS_ALLOCATE_UNITS;
			}
			break;

		case GS_ALLOCATE_UNITS:
			{
				// Deploy teams
				int nTeams = 4;
				int nWormsPerTeam = 4;

				// Calculate spacing of worms and teams
				float fSpacePerTeam = (float)nMapWidth / (float)nTeams;
				float fSpacePerWorm = fSpacePerTeam / (nWormsPerTeam * 2.0f);

				// Create teams
				for (int t = 0; t < nTeams; t++)
				{
					vecTeams.emplace_back(cTeam());
					float fTeamMiddle = (fSpacePerTeam / 2.0f) + (t * fSpacePerTeam);
					for (int w = 0; w < nWormsPerTeam; w++)
					{
						float fWormX = fTeamMiddle - ((fSpacePerWorm * (float)nWormsPerTeam) / 2.0f) + w * fSpacePerWorm;
						float fWormY = 0.0f;

						// Add worms to teams
						cWorm *worm = new cWorm(fWormX,fWormY);
						worm->nTeam = t;
						listObjects.push_back(unique_ptr<cWorm>(worm));
						vecTeams[t].vecMembers.push_back(worm);
						vecTeams[t].nTeamSize = nWormsPerTeam;
					}
					
					vecTeams[t].nCurrentMember = 0;
				}
				
				// Select players first worm for control and camera tracking
				pObjectUnderControl = vecTeams[0].vecMembers[vecTeams[0].nCurrentMember];
				pCameraTrackingObject = pObjectUnderControl;
				bShowCountDown = false;
				nNextState = GS_ALLOCATING_UNITS;
			}
			break;

		case GS_ALLOCATING_UNITS: // Wait for units to "parachute" in
			{
				if (bGameIsStable)
				{
					bEnablePlayerControl = true;
					bEnableComputerControl = false;
					fTurnTime = 15.0f;
					bZoomOut = false;
					nNextState = GS_START_PLAY;
				}
			}
			break;

		case GS_START_PLAY:
			{
				bShowCountDown = true;

				// If player has discharged weapon, or turn time is up, move on to next state
				if (bPlayerHasFired || fTurnTime <= 0.0f)
					nNextState = GS_CAMERA_MODE;
			}
			break;

		case GS_CAMERA_MODE: // Camera follows object of interest until the physics engine has settled
			{
				bEnableComputerControl = false;
				bEnablePlayerControl = false;
				bPlayerHasFired = false;
				bShowCountDown = false;
				fEnergyLevel = 0.0f;
				
				if (bGameIsStable) // Once settled, choose next worm
				{					
					// Get Next Team, if there is no next team, game is over
					int nOldTeam = nCurrentTeam;
					do {
						nCurrentTeam++;
						nCurrentTeam %= vecTeams.size();
					} while (!vecTeams[nCurrentTeam].IsTeamAlive());
					
					// Lock controls if AI team is currently playing	
					if (nCurrentTeam == 0) // Player Team
					{
						bEnablePlayerControl = true;	// Swap these around for complete AI battle
						bEnableComputerControl = false;
					}
					else // AI Team
					{
						bEnablePlayerControl = false;
						bEnableComputerControl = true;
					}

					// Set control and camera 
					pObjectUnderControl = vecTeams[nCurrentTeam].GetNextMember();
					pCameraTrackingObject = pObjectUnderControl;
					fTurnTime = 15.0f;
					bZoomOut = false;
					nNextState = GS_START_PLAY;

					// If no different team could be found...
					if (nCurrentTeam == nOldTeam)
					{
						// ...Game is over, Current Team have won!
						nNextState = GS_GAME_OVER1;
					}
				}
			}
			break;

			case GS_GAME_OVER1: // Zoom out and launch loads of missiles!
			{
				bEnableComputerControl = false;
				bEnablePlayerControl = false;
				bZoomOut = true;
				bShowCountDown = false;

				for (int i = 0; i < 100; i ++)
				{
					int nBombX = rand() % nMapWidth;
					int nBombY = rand() % (nMapHeight / 2);
					listObjects.push_back(unique_ptr<cMissile>(new cMissile(nBombX, nBombY, 0.0f, 0.5f)));
				}

				nNextState = GS_GAME_OVER2;
			}
			break;

			case GS_GAME_OVER2: // Stay here and wait for chaos to settle
			{
				bEnableComputerControl = false;
				bEnablePlayerControl = false;
				// No exit from this state!
			}
			break;

		}

		// AI State Machine
		if (bEnableComputerControl)
		{
			switch (nAIState)
			{
			case AI_ASSESS_ENVIRONMENT:
			{

				int nAction = rand() % 3;
				if (nAction == 0) // Play Defensive - move away from team
				{
					// Find nearest ally, walk away from them
					float fNearestAllyDistance = INFINITY; float fDirection = 0;
					cWorm *origin = (cWorm*)pObjectUnderControl;

					for (auto w : vecTeams[nCurrentTeam].vecMembers)
					{
						if (w != pObjectUnderControl)
						{
							if (fabs(w->px - origin->px) < fNearestAllyDistance)
							{
								fNearestAllyDistance = fabs(w->px - origin->px);
								fDirection = (w->px - origin->px) < 0.0f ? 1.0f : -1.0f;
							}
						}
					}

					if (fNearestAllyDistance < 50.0f)
						fAISafePosition = origin->px + fDirection * 80.0f;
					else
						fAISafePosition = origin->px;
				}

				if (nAction == 1) // Play Ballsy - move towards middle
				{
					cWorm *origin = (cWorm*)pObjectUnderControl;
					float fDirection = ((float)(nMapWidth / 2.0f) - origin->px) < 0.0f ? -1.0f : 1.0f;
					fAISafePosition = origin->px + fDirection * 200.0f;
				}

				if (nAction == 2) // Play Dumb - don't move
				{
					cWorm *origin = (cWorm*)pObjectUnderControl;
					fAISafePosition = origin->px;
				}

				// Clamp so dont walk off map
				if (fAISafePosition <= 20.0f) fAISafePosition = 20.0f;
				if (fAISafePosition >= nMapWidth - 20.0f) fAISafePosition = nMapWidth - 20.0f;	
				nAINextState = AI_MOVE;
			}
			break;

			case AI_MOVE:
			{
				cWorm *origin = (cWorm*)pObjectUnderControl;
				if (fTurnTime >= 8.0f && origin->px != fAISafePosition)
				{
					// Walk towards target until it is in range					
					if (fAISafePosition < origin->px && bGameIsStable)
					{
						origin->fShootAngle = -3.14159f * 0.6f;
						bAI_Jump = true;
						nAINextState = AI_MOVE;
					}

					if (fAISafePosition > origin->px && bGameIsStable)
					{
						origin->fShootAngle = -3.14159f * 0.4f;
						bAI_Jump = true;
						nAINextState = AI_MOVE;
					}
				}
				else
					nAINextState = AI_CHOOSE_TARGET;
			}
			break;

			case AI_CHOOSE_TARGET: // Worm finished moving, choose target
			{
				bAI_Jump = false;

				// Select Team that is not itself
				cWorm *origin = (cWorm*)pObjectUnderControl;
				int nCurrentTeam = origin->nTeam;
				int nTargetTeam = 0;
				do {
					nTargetTeam = rand() % vecTeams.size();
				} while (nTargetTeam == nCurrentTeam || !vecTeams[nTargetTeam].IsTeamAlive());

				// Aggressive strategy is to aim for opponent unit with most health	
				cWorm *mostHealthyWorm = vecTeams[nTargetTeam].vecMembers[0];
				for (auto w : vecTeams[nTargetTeam].vecMembers)
					if (w->fHealth > mostHealthyWorm->fHealth)
						mostHealthyWorm = w;

				pAITargetWorm = mostHealthyWorm;
				fAITargetX = mostHealthyWorm->px;
				fAITargetY = mostHealthyWorm->py;
				nAINextState = AI_POSITION_FOR_TARGET;						
			}
			break;

			case AI_POSITION_FOR_TARGET: // Calculate trajectory for target, if the worm needs to move, do so
			{
				cWorm *origin = (cWorm*)pObjectUnderControl;
				float dy = -(fAITargetY - origin->py);
				float dx = -(fAITargetX - origin->px);
				float fSpeed = 30.0f;
				float fGravity = 2.0f;

				bAI_Jump = false;

				float a = fSpeed * fSpeed*fSpeed*fSpeed - fGravity * (fGravity * dx * dx + 2.0f * dy * fSpeed * fSpeed);

				if (a < 0) // Target is out of range
				{
					if (fTurnTime >= 5.0f)
					{
						// Walk towards target until it is in range
						if (pAITargetWorm->px < origin->px && bGameIsStable)
						{
							origin->fShootAngle = -3.14159f * 0.6f;
							bAI_Jump = true;
							nAINextState = AI_POSITION_FOR_TARGET;
						}

						if (pAITargetWorm->px > origin->px && bGameIsStable)
						{
							origin->fShootAngle = -3.14159f * 0.4f;
							bAI_Jump = true;
							nAINextState = AI_POSITION_FOR_TARGET;
						}
					}
					else
					{
						// Worm is stuck, so just fire in direction of enemy!
						// Its dangerous to self, but may clear a blockage
						fAITargetAngle = origin->fShootAngle;
						fAITargetEnergy = 0.75f;
						nAINextState = AI_AIM;
					}
				}
				else
				{
					// Worm is close enough, calculate trajectory
					float b1 = fSpeed * fSpeed + sqrtf(a);
					//float b2 = fSpeed * fSpeed - sqrtf(a);

					float fTheta1 = atanf(b1 / (fGravity * dx)); // Max Height
					//float fTheta2 = atanf(b2 / (fGravity * dx)); // Min Height

					// We'll use max as its a greater chance of avoiding obstacles
					fAITargetAngle = fTheta1 - (dx > 0 ? 3.14159f : 0.0f);
					//float fFireX = cosf(fAITargetAngle);
					//float fFireY = sinf(fAITargetAngle);

					// AI is clamped to 3/4 power
					fAITargetEnergy = 0.75f;
					nAINextState = AI_AIM;
				}
			}
			break;

			case AI_AIM: // Line up aim cursor
			{
				cWorm *worm = (cWorm*)pObjectUnderControl;
				
				bAI_AimLeft = false;
				bAI_AimRight = false;
				bAI_Jump = false;
				
				if (worm->fShootAngle < fAITargetAngle)
					bAI_AimRight = true;
				else
					bAI_AimLeft = true;

				// Once cursors are aligned, fire - some noise could be
				// included here to give the AI a varying accuracy, and the
				// magnitude of the noise could be linked to game difficulty
				if (fabs(worm->fShootAngle - fAITargetAngle) <= 0.001f)
				{
					bAI_AimLeft = false;
					bAI_AimRight = false;
					fEnergyLevel = 0.0f;
					nAINextState = AI_FIRE;
				}
				else
					nAINextState = AI_AIM;
			}
			break;

			case AI_FIRE:
			{
				bAI_Energise = true;
				bFireWeapon = false;
				bEnergising = true;
				
				if (fEnergyLevel >= fAITargetEnergy)
				{
					bFireWeapon = true;
					bAI_Energise = false;
					bEnergising = false;
					bEnableComputerControl = false;
					nAINextState = AI_ASSESS_ENVIRONMENT;
				}
			}
			break;

			}			
		}

		if (!bCheat){
			// Decrease Turn Time
			fTurnTime -= fElapsedTime;
		}

		if (pObjectUnderControl != nullptr)
		{
			pObjectUnderControl->ax = 0.0f;

			if (pObjectUnderControl->bStable)
			{
				if ((bEnablePlayerControl && m_keys[L'Z'].bPressed) || (bEnableComputerControl && bAI_Jump))
				{
					float a = ((cWorm*)pObjectUnderControl)->fShootAngle;

					pObjectUnderControl->vx = 4.0f * cosf(a);
					pObjectUnderControl->vy = 8.0f * sinf(a);
					pObjectUnderControl->bStable = false;

					bAI_Jump = false;
				}

				if ((bEnablePlayerControl && m_keys[L'S'].bHeld) || (bEnableComputerControl && bAI_AimRight))
				{
					cWorm* worm = (cWorm*)pObjectUnderControl;
					worm->fShootAngle += 1.0f * fElapsedTime;
					if (worm->fShootAngle > 3.14159f) worm->fShootAngle -= 3.14159f * 2.0f;
				}

				if ((bEnablePlayerControl && m_keys[L'A'].bHeld) || (bEnableComputerControl && bAI_AimLeft))
				{
					cWorm* worm = (cWorm*)pObjectUnderControl;
					worm->fShootAngle -= 1.0f * fElapsedTime;
					if (worm->fShootAngle < -3.14159f) worm->fShootAngle += 3.14159f * 2.0f;
				}

				if ((bEnablePlayerControl && m_keys[VK_SPACE].bPressed))
				{
					bFireWeapon = false;
					bEnergising = true;
					fEnergyLevel = 0.0f;
				}

				if ((bEnablePlayerControl && m_keys[VK_SPACE].bHeld) || (bEnableComputerControl && bAI_Energise))
				{
					if (bEnergising)
					{
						fEnergyLevel += 0.75f * fElapsedTime;
						if (fEnergyLevel >= 1.0f)
						{
							fEnergyLevel = 1.0f;
							bFireWeapon = true;
						}
					}
				}

				if ((bEnablePlayerControl && m_keys[VK_SPACE].bReleased))
				{
					if (bEnergising)
					{
						bFireWeapon = true;
					}

					bEnergising = false;
				}
			}
		
			if (pCameraTrackingObject != nullptr)
			{
				fCameraPosXTarget = pCameraTrackingObject->px - ScreenWidth() / 2;
				fCameraPosYTarget = pCameraTrackingObject->py - ScreenHeight() / 2;
				fCameraPosX += (fCameraPosXTarget - fCameraPosX) * 15.0f * fElapsedTime;				
				fCameraPosY += (fCameraPosYTarget - fCameraPosY) * 15.0f * fElapsedTime;
			}

			if (bFireWeapon)
			{
				cWorm* worm = (cWorm*)pObjectUnderControl;

				// Get Weapon Origin
				float ox = worm->px;
				float oy = worm->py;

				// Get Weapon Direction
				float dx = cosf(worm->fShootAngle);
				float dy = sinf(worm->fShootAngle);

				// Create Weapon Object
				cMissile *m = new cMissile(ox, oy, dx * 40.0f * fEnergyLevel, dy * 40.0f * fEnergyLevel);
				pCameraTrackingObject = m;
				listObjects.push_back(unique_ptr<cMissile>(m));

				// Reset flags involved with firing weapon
				bFireWeapon = false;
				fEnergyLevel = 0.0f;
				bEnergising = false;
				bPlayerHasFired = true;

				if (rand() % 100 >= 50)
					bZoomOut = true;
			}
		}

		// Clamp map boundaries
		if (fCameraPosX < 0) fCameraPosX = 0;
		if (fCameraPosX >= nMapWidth - ScreenWidth()) fCameraPosX = nMapWidth - ScreenWidth();
		if (fCameraPosY < 0) fCameraPosY = 0;
		if (fCameraPosY >= nMapHeight - ScreenHeight()) fCameraPosY = nMapHeight - ScreenHeight();

		// Do 10 physics iterations per frame
		for (int z = 0; z < 10; z++)
		{
			// Update physics of all physical objects
			for (auto &p : listObjects)
			{
				// Apply Gravity
				p->ay += 2.0f;

				// Update Velocity
				p->vx += p->ax * fElapsedTime;
				p->vy += p->ay * fElapsedTime;

				// Update Position
				float fPotentialX = p->px + p->vx * fElapsedTime;
				float fPotentialY = p->py + p->vy * fElapsedTime;

				// Reset Acceleration
				p->ax = 0.0f;
				p->ay = 0.0f;

				p->bStable = false;

				// Collision Check With Map
				float fResponseX = 0;
				float fResponseY = 0;
				bool bCollision = p->CheckCollision(this, fPotentialX, fPotentialY, fResponseX, fResponseY);

				// Calculate magnitudes of response and velocity vectors
				float fMagVelocity = sqrtf(p->vx*p->vx + p->vy*p->vy);
				float fMagResponse = sqrtf(fResponseX*fResponseX + fResponseY*fResponseY);

				if (p->px < 0 || p->px > nMapWidth || p->py <0 || p->py > nMapHeight)
					p->bDead = true;
				if (isnanf(p->px) || isnanf(p->py))
					p->bDead = true;

				// Find angle of collision
				if (bCollision)
				{
					p->bStable = true;
					
					// Calculate reflection vector of objects velocity vector, using response vector as normal
					float dot = p->vx * (fResponseX / fMagResponse) + p->vy * (fResponseY / fMagResponse);

					// Use friction coefficient to dampen response (approximating energy loss)
					p->vx = p->fFriction * (-2.0f * dot * (fResponseX / fMagResponse) + p->vx);
					p->vy = p->fFriction * (-2.0f * dot * (fResponseY / fMagResponse) + p->vy);

					//Some objects will "die" after several bounces
					if (p->nBounceBeforeDeath > 0)
					{
						p->nBounceBeforeDeath--;
						p->bDead = p->nBounceBeforeDeath == 0;
						if (p->bDead)
						{
							// Action upon object death
							// = 0 Nothing
							// > 0 Explosion
							// < 0 Build!
							int nResponse = p->BounceDeathAction();
							if (nResponse > 0)
							{
								Boom(p->px, p->py, nResponse);
								pCameraTrackingObject = nullptr;
							} else if (nResponse<0){
								// Build Terrain
								//CircleBresenham(p->px, p->py, -nResponse, 1);
								//CircleBresenham(fPotentialX, fPotentialY, -nResponse, 1);
								// Create a pixel
								//listObjects.push_back(unique_ptr<cPixel>(new cPixel(p->px, p->py)));
								// Create four pixels
								listObjects.push_back(unique_ptr<cPixel>(new cPixel(p->px, p->py)));
								listObjects.push_back(unique_ptr<cPixel>(new cPixel(p->px + 1.0f, p->py)));
								listObjects.push_back(unique_ptr<cPixel>(new cPixel(p->px, p->py + 1.0f)));
								listObjects.push_back(unique_ptr<cPixel>(new cPixel(p->px + 1.0f, p->py + 1.0f)));
							}						
						}
					}
				}
				else
				{
					p->px = fPotentialX;
					p->py = fPotentialY;
				}

				if (p->CheckStability(bCollision, fResponseX, fResponseY, fMagResponse, fMagVelocity))
					p->bStable = true;

				// Some objects will "die" when they become stable
				if (p->bStable && p->bDieWhenStable){
					p->bDead = true;

					// If object died, work out what to do next
					if (p->bDead)
					{
						// Action upon object death
						// = 0 Nothing
						// > 0 Build
						// < 0 Explode!
						int nResponse = p->StableDeathAction();
						if (nResponse > 0){
							if (nResponse == 1)
								DrawMap(p->px, p->py, 1);
							else
								CircleBresenham(p->px, p->py, nResponse, 1);
						}else if (nResponse < 0){
							Boom(p->px, p->py, -nResponse);
						}
					}
				}
			}

			// Remove dead objects from the list, so they are not processed further. As the object
			// is a unique pointer, it will go out of scope too, deleting the object automatically. Nice :-)
			listObjects.remove_if([](unique_ptr<cPhysicsObject> &o){return o->bDead;});
		}

		// Draw Landscape
		if (!bZoomOut)
		{
			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
				{
					switch (map[(y + (int)fCameraPosY)*nMapWidth + (x + (int)fCameraPosX)])
					{
					case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
					case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
					case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
					case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
					case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
					case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
					case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
					case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;
					case 0:	Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
					case 1:	Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN);	break;
					}
				}

			// Draw objects - they draw themselves
			for (auto &p : listObjects)
			{
				p->Draw(this, fCameraPosX, fCameraPosY);

				cWorm* worm = (cWorm*)pObjectUnderControl;
				if (p.get() == worm)
				{
					// Draw Crosshair
					float cx = worm->px + 8.0f * cosf(worm->fShootAngle) - fCameraPosX;
					float cy = worm->py + 8.0f * sinf(worm->fShootAngle) - fCameraPosY;

					Draw(cx, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx + 1, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx - 1, cy, PIXEL_SOLID, FG_BLACK);
					Draw(cx, cy + 1, PIXEL_SOLID, FG_BLACK);
					Draw(cx, cy - 1, PIXEL_SOLID, FG_BLACK);

					//DrawString(cx - 3, cy - 3, to_wstring(bEnergyLevel), FG_WHITE);
					for (int i = 0; i < 11 * fEnergyLevel; i++)
					{
						Draw(worm->px - 5 + i - fCameraPosX, worm->py - 12 - fCameraPosY, PIXEL_SOLID, FG_GREEN);
						Draw(worm->px - 5 + i - fCameraPosX, worm->py - 11 - fCameraPosY, PIXEL_SOLID, FG_RED);
					}
				}
			}
		}
		else
		{
			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
				{
					float fx = (float)x/(float)ScreenWidth() * (float)nMapWidth;
					float fy = (float)y/(float)ScreenHeight() * (float)nMapHeight;

					switch (map[((int)fy)*nMapWidth + ((int)fx)])
					{
					case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
					case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
					case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
					case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
					case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
					case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
					case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
					case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;
					case 0:	Draw(x, y, PIXEL_SOLID, FG_CYAN);break;
					case 1:	Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN);	break;
					}
				}

			for (auto &p : listObjects)
				p->Draw(this, p->px-(p->px / (float)nMapWidth) * (float)ScreenWidth(), 
					p->py-(p->py / (float)nMapHeight) * (float)ScreenHeight(), true);
		}

	
		// Check For game state stability
		bGameIsStable = true;
		for (auto &p : listObjects)
			if (!p->bStable)
			{
				bGameIsStable = false;
				break;
			}

		// This little marker is handy for debugging
		if (bGameIsStable)
			Fill(2, 2, 6, 6, PIXEL_SOLID, FG_RED);

		// Draw Team Health Bars
		for (size_t t = 0; t < vecTeams.size(); t++)
		{
			float fTotalHealth = 0.0f;
			float fMaxHealth = (float)vecTeams[t].nTeamSize;
			for (auto w : vecTeams[t].vecMembers) // Accumulate team health
				fTotalHealth += w->fHealth;

			int cols[] = { FG_RED, FG_BLUE, FG_MAGENTA, FG_GREEN };
			Fill(4, 4 + t * 4, (fTotalHealth / fMaxHealth) * (float)(ScreenWidth() - 8) + 4, 4 + t * 4 + 3, PIXEL_SOLID, cols[t]);
		}

		// Mystery Code !! Displays Seven-Segment Display for countdown timer
		// Display Turn Time - Check out the discord #challenges for explanation!
		// Thanks to all those awesome discord chatters who took part! I've kept
		// this minimised and obfuscated to maintain the spirit of the challenge!
		if (bShowCountDown)
		{
			wchar_t d[]=L"w$]m.k{\%\x7Fo";int tx=4,ty=vecTeams.size()*4+8;
			for(int r=0;r<13;r++){for(int c=0;c<((fTurnTime<10.0f)?1:2);c++){
			int a=to_wstring(fTurnTime)[c]-48;if(!(r%6)){DrawStringAlpha(tx,
			ty,wstring((d[a]&(1<<(r/2))?L" #####  ":L"        ")),FG_BLACK);
			tx+=8;}else{DrawStringAlpha(tx,ty,wstring((d[a]&(1<<(r<6?1:4))?
			L"#     ":L"      ")),FG_BLACK);tx+=6;DrawStringAlpha(tx,ty,wstring
			((d[a]&(1<<(r<6?2:5))?L"# ":L"  ")),FG_BLACK);tx+=2;}}ty++;tx=4;}
		}

		// Update State Machine
		nGameState = nNextState;
		nAIState = nAINextState;

		return true;
	}

	void Render(){

	}

	void DrawMap(int x, int y, char val){
		if (y >= 0 && y < nMapHeight && x >= 0 && x < nMapWidth)
			map[y * nMapWidth + x] = val;
	}

	void CircleBresenham(int xc, int yc, int r, char val)
	{
		// Taken from wikipedia
		int x = 0;
		int y = r;
		int p = 3 - 2 * r;
		if (!r) return;

		auto drawline = [&](int sx, int ex, int ny)
		{
			for (int i = sx; i < ex; i++)
				DrawMap(i, ny, val);
		};

		while (y >= x) 
		{
			// Modified to draw scan-lines instead of edges
			drawline(xc - x, xc + x, yc - y);
			drawline(xc - y, xc + y, yc - x);
			drawline(xc - x, xc + x, yc + y);
			drawline(xc - y, xc + y, yc + x);
			if (p < 0) p += 4 * x++ + 6;
			else p += 4 * (x++ - y--) + 10;
		}
	}

	void Boom(float fWorldX, float fWorldY, float fRadius)
	{
		// Erase Terrain to form crater
		CircleBresenham(fWorldX, fWorldY, fRadius, 0);

		// Shockwave other entities in range
		for (auto &p : listObjects)
		{
			float dx = p->px - fWorldX;
			float dy = p->py - fWorldY;
			float fDist = sqrt(dx*dx + dy*dy);
			if (fDist < 0.0001f) fDist = 0.0001f;
			if (fDist < fRadius)
			{
				p->vx = (dx / fDist) * fRadius;
				p->vy = (dy / fDist) * fRadius;
				p->Damage(((fRadius - fDist) / fRadius) * 0.8f); // Corrected ;)
				p->bStable = false;				
			}
		}

		// Launch debris
		for (int i = 0; i < (int)fRadius; i++)
			listObjects.push_back(unique_ptr<cDebris>(new cDebris(fWorldX, fWorldY)));
	}

	// Taken from Perlin Noise Video https://youtu.be/6-0UaeJBumA
	void PerlinNoise1D(int nCount, float *fSeed, int nOctaves, float fBias, float *fOutput)
	{
		// Used 1D Perlin Noise
		for (int x = 0; x < nCount; x++)
		{
			float fNoise = 0.0f;
			float fScaleAcc = 0.0f;
			float fScale = 1.0f;

			for (int o = 0; o < nOctaves; o++)
			{
				int nPitch = nCount >> o;
				int nSample1 = (x / nPitch) * nPitch;
				int nSample2 = (nSample1 + nPitch) % nCount;
				float fBlend = (float)(x - nSample1) / (float)nPitch;
				float fSample = (1.0f - fBlend) * fSeed[nSample1] + fBlend * fSeed[nSample2];
				fScaleAcc += fScale;
				fNoise += fSample * fScale;
				fScale = fScale / fBias;
			}

			// Scale to seed range
			fOutput[x] = fNoise / fScaleAcc;
		}
	}

	void CreateMap()
	{
		// Used 1D Perlin Noise
		float *fSurface = new float[nMapWidth];
		float *fNoiseSeed = new float[nMapWidth];
		for (int i = 0; i < nMapWidth; i++) 
			fNoiseSeed[i] = (float)rand() / (float)RAND_MAX;

		fNoiseSeed[0] = 0.5f;
		PerlinNoise1D(nMapWidth, fNoiseSeed, 8, 2.0f, fSurface);

		for (int x = 0; x < nMapWidth; x++)
			for (int y = 0; y < nMapHeight; y++)
			{
				if (y >= fSurface[x] * nMapHeight)
					map[y * nMapWidth + x] = 1;
				else
				{
					// Shade the sky according to altitude - we only do top 1/3 of map
					// as the Boom() function will just paint in 0 (cyan)
					if ((float)y < (float)nMapHeight / 3.0f)
						map[y * nMapWidth + x] = (-8.0f * ((float)y / (nMapHeight / 3.0f))) -1.0f;
					else
						map[y * nMapWidth + x] = 0;
				}
			}
	
		delete[] fSurface;
		delete[] fNoiseSeed;
	}


};



int main()
{
	OneLoneCoder_Worms game;
	game.ConstructConsole(256, 160, 6, 6);
	game.Start();
	return 0;
}

