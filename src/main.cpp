#include "precompiled.h"
#include "ciextra.h"
#if 0

Perlin perlin(2);
const int wsz=600;
Vec2f perlin_lookup[wsz][wsz];
float anim=0;

struct Part{
	Vec2f pos;
	Vec2f oldPos;
	Vec2f vel;
	Part(Vec2f pos){
		this->pos=pos;
		this->oldPos=pos;
		this->vel = Vec2f::zero();
	}
};
struct PSys {
	vector<Part> parts;
	void update()
	{
		foreach(auto& part, parts)
		{
			auto d2D = Vec2f(part.pos.x/wsz, part.pos.y/wsz)*2;
			float dx=perlin.fBm(d2D.x, d2D.y, 0+anim);
			float dy=perlin.fBm(d2D.x, d2D.y, .5+anim);
			part.oldPos = part.pos;
			part.pos += Vec2f(dx, dy)*100;
			//part.pos+=part.vel;
		}
	};
} psys;

struct SApp : AppBasic {
	void setup()
	{
		createConsole();
		setWindowSize(wsz, wsz);
		//for(int x=0;x<wsz;x++) for(int y=0;y<wsz;y++) perlin_lookup
	}
	void mouseDown(MouseEvent e)
	{
	}
	void draw()
	{
		anim+=.001;
		gl::clear(Color(0, 0, 0));
		psys=PSys();
		for(int i = 0; i < max(getWindowWidth(), getWindowHeight()); i++)
		{
			if(i%10!=0)continue;
			if(i<getWindowHeight()){
				psys.parts.push_back(Part(Vec2f(0,i)));
				psys.parts.push_back(Part(Vec2f(getWindowWidth(),i)));
			}
			if(i<getWindowWidth()){
				psys.parts.push_back(Part(Vec2f(i,0)));
				psys.parts.push_back(Part(Vec2f(i,getWindowHeight())));
			}
		}
		gl::enableAdditiveBlending();
		//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		gl::color(Color::gray(.05));
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(20);
		glBegin(GL_LINES);
		for(int i = 0; i < 50; i++)
		{
			psys.update();
			foreach(auto& part, psys.parts)
			{
				gl::vertex(part.pos);
				gl::vertex(part.oldPos);
			}
		}
		glEnd();
	}
};

CINDER_APP_BASIC(SApp, RendererGl)
#endif