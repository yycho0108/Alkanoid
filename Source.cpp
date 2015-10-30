#include <Windows.h>
#include <iostream>
#include <functional>
#include <random>
#include <ctime>
#include <list>
ATOM RegisterCustomClass(HINSTANCE& hInstance);
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void StartGame(HWND hWnd);
void EndGame(HWND hWnd);
LPCTSTR Title = L"Alkanoid";
HINSTANCE g_hInst;
HWND hMainWnd;
HBITMAP MemBit;
int SWidth, SHeight;
int NumBall = 1;
std::function<int()> RandInt = std::bind(std::uniform_int_distribution<int>{ INT_MIN, INT_MAX }, std::default_random_engine{ (unsigned int(time(0))) });
std::function<double()> RandDbl = std::bind(std::uniform_real_distribution<double>{-1.0,1.0}, std::default_random_engine{ (unsigned int(time(0))) });

struct Vec2D
{
	double x, y;
	
	double LengthSQ(){ return x*x + y*y; }
	double Length(){ return sqrt(LengthSQ());}
	Vec2D Rotate(double theta){ return{ x*cos(theta) - y*sin(theta), x*sin(theta) + y*cos(theta) }; }
	Vec2D Normalize(){ return{ x / Length(), y / Length() }; }
	Vec2D operator+(Vec2D& Other){ return{ x + Other.x, y + Other.y }; }
	Vec2D& operator+=(Vec2D& Other){ x += Other.x; y += Other.y; return *this; }
	Vec2D operator-(Vec2D& Other){ return{ x -Other.x, y -Other.y }; }
	Vec2D& operator-=(Vec2D& Other){ x -= Other.x; y -= Other.y; return *this; }
	Vec2D operator*(double Mult){ return{ x*Mult, y*Mult }; }
	Vec2D& operator*=(double Mult){ x *= Mult; y *= Mult; return *this; }
	Vec2D operator/(double Div){ return{ x / Div, y / Div }; }
	Vec2D& operator/=(double Div){ x /= Div, y /= Div; return *this; }
	double DotProduct(Vec2D& Other){ return x*Other.x + y*Other.y; }
	Vec2D(double x, double y) :x{ x }, y{ y }{};
	Vec2D() :x{ 0 }, y{ 0 }{};

};
class Entity
{
protected:
	Vec2D Pos;
	Vec2D Vel;
	Vec2D Size;
	double angle;
	Entity(Vec2D P = Vec2D(), Vec2D V = Vec2D(), Vec2D S = Vec2D(), double angle = 0) :Pos{ P }, Vel{ V }, Size{ S }, angle{ angle }{};
	virtual void Move(){ Pos += Vel; }
	virtual void Print(HDC) = 0;
};

class Block : public Entity
{
	HBRUSH myColorBrush;
	COLORREF Color;
public:
	static std::list<Block*> Blocks;
	Block(Vec2D P = Vec2D(), Vec2D V = Vec2D(), Vec2D S = Vec2D(50, 50), COLORREF C = RGB(RandInt() % 255, RandInt() % 255, RandInt() % 255)) :Entity(P, V, S), Color{ C }
	{
		myColorBrush = CreateSolidBrush(Color);
	}
	~Block()
	{
		DeleteObject(myColorBrush);
	}
	virtual void Print(HDC hdc)
	{
		SelectObject(hdc, GetStockObject(NULL_PEN));
		SelectObject(hdc, myColorBrush);
		Rectangle(hdc, Pos.x - Size.x, Pos.y - Size.y, Pos.x + Size.x, Pos.y + Size.y);
	}
	virtual void Move(){}
	bool isPtOnMe(int ax, int ay)
	{
		if (abs(ax - Pos.x) > Size.x || abs(ay - Pos.y) > Size.y) return false;
		else return true;
	}
};
std::list<Block*> Block::Blocks;
enum { ID_CHILDWINDOW = 100 };


class Ball : public Entity
{
public:
	bool Alive;
	enum dir{LEFT,DOWN,RIGHT,UP};
	virtual void Print(HDC hdc)
	{
		Ellipse(hdc, Pos.x - Size.x, Pos.y - Size.y, Pos.x + Size.x, Pos.y + Size.y);
	}
	virtual void Move()
	{
		Pos += Vel;
		if (Pos.x < 0)
		{
			Bounce(dir::LEFT);
		}
		if (Pos.x > SWidth)
		{
			Bounce(dir::RIGHT);
		}
		if (Pos.y < 0)
		{
			Bounce(dir::UP);
		}
		if (Pos.y > SHeight)
		{
			Alive = false;
		}
	}
	bool isBlockThere(Block* B)
	{
		Vec2D HS = Size / 2;
		if (B->isPtOnMe(Pos.x-HS.x, Pos.y)) //left
		{
			Bounce(dir::LEFT);
			return true;
		}
		if (B->isPtOnMe(Pos.x+HS.x, Pos.y)) //Right
		{
			Bounce(dir::RIGHT);
			return true;
		}
		if (B->isPtOnMe(Pos.x, Pos.y-HS.y)) //Up
		{
			Bounce(dir::UP);
			return true;
		}
		if (B->isPtOnMe(Pos.x - HS.x, Pos.y+HS.y)) //Down
		{
			Bounce(dir::DOWN);
			return true;
		}
		return false;
	}
	bool isBallThere(Ball* B)
	{
		Vec2D Rel(Pos.x - B->Pos.x, Pos.y- B->Pos.y);
		if (Rel.LengthSQ() < B->Size.x*B->Size.y + Size.x*Size.y)
		{
			Bounce(Rel);
			return true;
			//°ãÄ§
		}
		else return false;
	}
	void Bounce(dir D)
	{
		switch (D)
		{
		case dir::LEFT:
			Vel.x = abs(Vel.x);
			break;
		case dir::RIGHT:
			Vel.x = -abs(Vel.x);
			break;
		case dir::UP:
			Vel.y = abs(Vel.y);
			break;
		case dir::DOWN:
			Vel.y = -abs(Vel.y);
			//DIE
			break;

		}
	}
	void Bounce(Vec2D Axis)
	{
		Vec2D n = Axis.Normalize();
		Vel = Vel - n * 2 * (Vel.DotProduct(n));
		Vel = (Vel + n*3).Normalize() * Vel.Length();
	}
public:
	Ball(Vec2D P = Vec2D(SWidth/2,SHeight*0.9-30), Vec2D V = Vec2D(10*RandDbl(),-10*abs(RandDbl())).Normalize()*3, Vec2D S = Vec2D(20,20), double angle = 0) :Entity(P, V, S, angle)
	{
		angle = atan2(-V.y, V.x);
	}
} *Sphere;

class Racket : public Ball
{
public:
	bool Left, Right, Up, Down;
	void Impulse(dir D, bool Push)
	{
		switch (D)
		{
		case dir::LEFT:
			Left = Push;
			break;
		case dir::RIGHT:
			Right = Push;
			break;
		case dir::UP:
			Up = Push;
			break;
		case dir::DOWN:
			Down = Push;
			break;
		}
	}
	virtual void Move()
	{
		if (Left) Vel.x -= 0.5;
		if (Right) Vel.x += 0.5;
		if (Down) Vel.y += 0.5;
		if (Up) Vel.y -= 0.5;

		Pos += Vel;
		if (Pos.x < 0)
		{
			Bounce(dir::LEFT);
		}
		if (Pos.x > SWidth)
		{
			Bounce(dir::RIGHT);
		}
		if (Pos.y < 0)
		{
			Bounce(dir::UP);
		}
		if (Pos.y > SHeight)
		{
			Bounce(dir::DOWN);
		}
		Vel *= 0.9;
	}
	Racket(Vec2D P = Vec2D(SWidth / 2, SHeight*0.9+50), Vec2D V = Vec2D(), Vec2D S = Vec2D(50, 50), double angle = 0):Ball(P, V, S, angle)
	{
		Left = Right = Up = Down = false;
	}
} *Me;

ATOM RegisterCustomClass(HINSTANCE& hInstance)
{
	WNDCLASS wc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = Title;
	wc.lpszMenuName = NULL;
	wc.style = CS_VREDRAW | CS_HREDRAW;
	return RegisterClass(&wc);
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	g_hInst = hInstance;
	RegisterCustomClass(hInstance);
	hMainWnd = CreateWindow(Title, Title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	ShowWindow(hMainWnd, nCmdShow);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_CREATE:
	{
		StartGame(hWnd);
	}
		break;
	case WM_TIMER:
	{
		HDC hdc = GetDC(hWnd);
		HDC MemDC = CreateCompatibleDC(hdc);
		HBITMAP OldBit = (HBITMAP)SelectObject(MemDC, MemBit);

		RECT R{ 0, 0, SWidth, SHeight };
		FillRect(MemDC, &R, (HBRUSH)GetStockObject(WHITE_BRUSH));
		
		for (int i = 0; i < 3; ++i)
		{
			if (Sphere->isBallThere(Me))
				Sphere->Move();
			Sphere->Move();
			Sphere->Print(MemDC);
			Me->Move();
			Me->Print(MemDC);
		}

		for (auto i = Block::Blocks.begin(); i != Block::Blocks.end();)
		{
			Block* b = *i++;
			if (Sphere->isBlockThere(b))
			{
				Block::Blocks.remove(b);
				delete b;
			}
			else
			{
				b->Print(MemDC);
			}
		}
		//Draw Stuff
		if (!Sphere->Alive)
		{
			if (--NumBall < 1)
			{
				EndGame(hWnd);
				if (MessageBox(hWnd, L"RESTART?", L"GAME OVER", MB_OKCANCEL) == IDOK)
					StartGame(hWnd);
				else DestroyWindow(hWnd);
			}
		}
		SelectObject(MemDC, OldBit);
		DeleteObject(MemDC);
		ReleaseDC(hWnd, hdc);
		InvalidateRect(hWnd, NULL, FALSE);
	}
	break;
	case WM_LBUTTONDOWN:
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);

		for (auto i = Block::Blocks.begin(); i != Block::Blocks.end();)
		{
			Block* b = *i++;
			if (b->isPtOnMe(x, y))
			{
				Block::Blocks.remove(b);
				delete b;
			}
		}
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case 'A':
		case VK_LEFT:
			Me->Impulse(Ball::dir::LEFT, true);
			break;
		case 'D':
		case VK_RIGHT:
			Me->Impulse(Ball::dir::RIGHT, true);
			break;
		case 'S':
		case VK_DOWN:
			Me->Impulse(Ball::dir::DOWN, true);
			break;
		case 'W':
		case VK_UP:
			Me->Impulse(Ball::dir::UP, true);
			break;
		case VK_ESCAPE:
			EndGame(hWnd);
			if (MessageBox(hWnd, L"RESTART?", L"GAME OVER", MB_OKCANCEL) == IDOK)
				StartGame(hWnd);
			break;
		}
		break;
	}
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case 'A':
		case VK_LEFT:
			Me->Impulse(Ball::dir::LEFT, false);
			break;
		case 'D':
		case VK_RIGHT:
			Me->Impulse(Ball::dir::RIGHT, false);
			break;
		case 'S':
		case VK_DOWN:
			Me->Impulse(Ball::dir::DOWN, false);
			break;
		case 'W':
		case VK_UP:
			Me->Impulse(Ball::dir::UP, false);
			break;
		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		HDC MemDC = CreateCompatibleDC(hdc);
		HBITMAP OldBit = (HBITMAP)SelectObject(MemDC, MemBit);

		BitBlt(hdc, 0, 0, SWidth, SHeight, MemDC, 0, 0, SRCCOPY);

		SelectObject(MemDC, OldBit);
		DeleteObject(MemDC);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_SIZE:
	{
		RECT R;
		GetClientRect(hWnd, &R);
		SHeight = R.bottom - R.top;
		SWidth = R.right - R.left;
	}
	break;
	case WM_DESTROY:
		EndGame(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

void StartGame(HWND hWnd)
{

	HDC hdc = GetDC(hWnd);
	MemBit = CreateCompatibleBitmap(hdc, GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, VERTRES));
	SetTimer(hWnd, 0, 10, NULL);
	SendMessage(hWnd, WM_SIZE, NULL, NULL);
	for (int i = 0; i < SHeight / 3; i += 30)
	{
		for (int j = 0; j < SWidth; j += 70)
		{
			Block::Blocks.push_back(new Block{ { (double)j, (double)i }, { 0, 0 }, { 35, 15 } });
		}
	}
	Me = new Racket();
	Sphere = new Ball();
	NumBall = 1;
	Me->Print(hdc);
	Sphere->Print(hdc);
	ReleaseDC(hWnd, hdc);
}

void EndGame(HWND hWnd)
{
	KillTimer(hWnd, 0);
	DeleteObject(MemBit);
	for (auto b : Block::Blocks)
		delete b;
	delete Me;
	Me = nullptr;
	delete Sphere;
	Sphere = nullptr;
	Block::Blocks.clear();
}