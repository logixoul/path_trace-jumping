#include "precompiled.h"
#include "ciextra.h"
#if 0

Perlin perlin(2);
const int wsz=600;
vec2 perlin_lookup[wsz][wsz];
float anim=0;

struct Part{
	vec2 pos;
	vec2 oldPos;
	vec2 vel;
	Part(vec2 pos){
		this->pos=pos;
		this->oldPos=pos;
		this->vel = vec2::zero();
	}
};
struct PSys {
	vector<Part> parts;
	void update()
	{
		foreach(auto& part, parts)
		{
			auto d2D = vec2(part.pos.x/wsz, part.pos.y/wsz)*2;
			float dx=perlin.fBm(d2D.x, d2D.y, 0+anim);
			float dy=perlin.fBm(d2D.x, d2D.y, .5+anim);
			part.oldPos = part.pos;
			part.pos += vec2(dx, dy)*100;
			//part.pos+=part.vel;
		}
	};
} psys;

struct SApp : App {
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
				psys.parts.push_back(Part(vec2(0,i)));
				psys.parts.push_back(Part(vec2(getWindowWidth(),i)));
			}
			if(i<getWindowWidth()){
				psys.parts.push_back(Part(vec2(i,0)));
				psys.parts.push_back(Part(vec2(i,getWindowHeight())));
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