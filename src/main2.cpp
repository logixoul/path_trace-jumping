#include "precompiled.h"
#include "ciextra.h"
#include <cinder/gl/Fbo.h>
#include <cinder/Camera.h>
#include "util.h"
#include "sat.h"
Perlin perlin(5);
const int wsz=600;
vec2 perlin_lookup[wsz][wsz];
float anim=0;
int splitAge=1000;
Array2D<vec2> interact_map(wsz,wsz);
Area world(0,0, wsz,wsz);

struct Part{
	float index;int age;
	bool slowlyDie;
	struct Pos
	{
		vec2 vec;
		vec2 vel;
		bool wrap;
		Pos(vec2 vec, bool wrap){ this->vec=vec; this->wrap=wrap;vel=vec2();}
	};
	deque<Pos> pos;
	Part(vec2 pos,float index){
		this->pos.push_back(Pos(pos, false));
		this->index=index;
		age=Rand::randInt(0,splitAge);
		slowlyDie=false;
	}
	void update()
	{
		if(slowlyDie)
		{
			if(pos.size() != 0)
				pos.pop_front();
			return;
		}
		auto d2D = vec2(pos.back().vec.x/wsz, pos.back().vec.y/wsz)*2.0f;
		float dx=sin(anim)*perlin.fBm(d2D.x, d2D.y, 0+anim+index);
		float dy=cos(anim)*perlin.fBm(d2D.x, d2D.y, .5+anim+index);
		auto d = vec2(dx, dy)*10.0f;
		//d += interact_map.limitGet(pos.back().vec);
		pos.back().vel+=d;//d.normalized()*max(10.f,d.length());
		pos.back().vel*=.999;
		index+=std::log(1+length(pos.back().vel))*.00001;
		auto newPos = pos.back().vec+pos.back().vel;
		auto nonWrapped=newPos;
		if(newPos.x<0) newPos.x+=wsz;
		if(newPos.y<0) newPos.y+=wsz;
		if(newPos.x>=wsz) newPos.x-=wsz;
		if(newPos.y>=wsz) newPos.y-=wsz;
		pos.push_back(Part::Pos(newPos, newPos!=nonWrapped));
		if(pos.size()>100)
			pos.pop_front();
		
	}
};
struct PSys {
	deque<Part> parts;
	void update()
	{
		float index=0;
		vector<Part> newParts;
		foreach(auto& part, parts)
		{
			part.update();
			part.age++;
			if(part.age>splitAge&&!part.slowlyDie)
			{
				auto newPart = Part(part.pos.front().vec, part.index+.1);
				newPart.pos.front().vel=glm::rotate(newPart.pos.front().vel, -1.0f);
				part.pos.front().vel = glm::rotate(part.pos.front().vel, 1.0f);
				newParts.push_back(newPart);
				part.age=INT_MIN;
			}
		}
		parts.insert(parts.end(), newParts.begin(), newParts.end());
		for(int i = 1000; i < parts.size(); i++) parts[i].slowlyDie=true;
		parts.erase(remove_if(parts.begin(), parts.end(), [&](Part& part) { return part.pos.size()==0; }), parts.end());
		cout<<parts.size()<<endl;
	};
} psys;
bool showInteractMap; bool captureMouse=true;vec3 worldUp(0,1,0);
struct SApp : App {
	ci::CameraPersp cam;
	void setup()
	{
		createConsole();
		setWindowSize(wsz, wsz);
		addParticles();
		//for(int x=0;x<wsz;x++) for(int y=0;y<wsz;y++) perlin_lookup
		pindex=0;
		gl::setMatricesWindowPersp(getWindowSize());
		cam = ci::CameraPersp();
		updownRot=0;leftrightRot=0;
		hideCursor();
		camPos=vec3(0,0, 0);
		yvel=0;
		cam.setFov(100);
	}
	float pindex;
		
	void addParticles()
	{
		float pindexstep=.003;
		for(int i = 0; i < max(getWindowWidth(), getWindowHeight()); i++)
		{
			if(i%3!=0)continue;
			if(i<getWindowHeight()){
				psys.parts.push_back(Part(vec2(0,i),pindex));
				pindex+=pindexstep;
				psys.parts.push_back(Part(vec2(getWindowWidth(),i),pindex));
				pindex+=pindexstep;
			}
			if(i<getWindowWidth()){
				psys.parts.push_back(Part(vec2(i,0),pindex));
				pindex+=pindexstep;
				psys.parts.push_back(Part(vec2(i,getWindowHeight()),pindex));
				pindex+=pindexstep;
			}
		}
	}
	float updownRot, leftrightRot; vec3 camPos; float yvel; ivec2 oldMPos;
	void mouseMove(MouseEvent e)
	{
		if(e.getPos()==oldMPos)return;//recursion
		
		quat o = cam.getOrientation();
		leftrightRot-=e.getX()-oldMPos.x;
		updownRot+=e.getY()-oldMPos.y;
		
		if(captureMouse)
		{
			centerMouse();
		}else
			oldMPos=e.getPos();
		//cam.setOrientation(Quatf(0, leftrightRot/1000.0, 0)*Quatf(updownRot/1000.0, 0, 0));
		//cam.setWorldUp(vec3(0, 0, 1));
	}
	void centerMouse() { auto newCursorPos = getWindowPos() + ivec2(wsz,wsz)/2;SetCursorPos(newCursorPos.x, newCursorPos.y); oldMPos=newCursorPos-getWindowPos(); }
	vector<bool> keys;
	void keyDown(KeyEvent e)
	{
		if(e.getChar()=='i')
			showInteractMap = !showInteractMap;
		if(e.getCode()==KeyEvent::KEY_ESCAPE)
			quit();
		if(e.getChar()=='c')
			captureMouse = !captureMouse;
		if(e.getChar()==' ')
			yvel=10;
	}
	mat3 getCameraRotation() {
		mat4 rot = glm::eulerAngleYXZ(updownRot / 1000.0f, leftrightRot / 1000.0f, 0.0f);
		return mat3(rot);
		//return mat3::createRotation(vec3());
	}

	void updateCam()
	{
		//mat3 cameraRotation = Matrix.CreateRotationX(leftrightRot) * Matrix.CreateRotationY(updownRot);
		auto cameraRotation = this->getCameraRotation();
		vec3 cameraOriginalTarget = vec3(0, 0, 1);
		vec3 cameraRotatedTarget = cameraRotation * cameraOriginalTarget;
		vec3 cameraFinalTarget = camPos + cameraRotatedTarget;

		vec3 cameraOriginalUpVector = worldUp;
		vec3 cameraRotatedUpVector = cameraRotation * cameraOriginalUpVector;
		//cam.setEyePoint(camPos);
		//cam.setViewDirection(cameraFinalTarget-camPos);
		//cam.setWorldUp(cameraRotatedUpVector);
		cam.lookAt(camPos, cameraFinalTarget, cameraRotatedUpVector);
 		//viewMatrix = Matrix.CreateLookAt(cameraPosition, cameraFinalTarget, cameraRotatedUpVector);
	}
	ci::Timer timer;
	void swapYZ()
	{
		glMultMatrixf(Matrix44f(
				vec3(1, 0, 0),
				vec3(0, 0, 1),
				vec3(0, 1, 0)));
	}
	void draw()
	{
		timer.stop();
		double elapsed=timer.getSeconds();
		timer.start();
		updateCam();
		gl::drawCoordinateFrame();
		cam.setEyePoint(camPos);
		yvel-=4*elapsed; 
		camPos.y+=yvel*elapsed*40.0;
		if(camPos.y<50)camPos.y=50;

		auto cameraRotation = this->getCameraRotation();
		const float camSpeed=10;
		vec3 moveDir = normalize(vec3(cam.getViewDirection().x, 0, cam.getViewDirection().z)) * camSpeed;
		if(keys['w'])
			camPos+=moveDir;
		if(keys['s'])
			camPos-=moveDir;
		if(keys['a'])
			camPos-=cross(moveDir, worldUp);
		if(keys['d'])
			camPos+=cross(moveDir, worldUp);
		const float fff=5;
		if(keys['i'])updownRot+=fff;
		if(keys['k'])updownRot-=fff;
		if(keys['j'])leftrightRot+=fff;
		if(keys['l'])leftrightRot-=fff;
		

		if(captureMouse)
			centerMouse();

		//cam.setCenterOfInterestPoint(vec3(getMousePos().x, wsz-getMousePos().y, 0));
		cam.setFarClip(10000);
		gl::setMatrices(cam);

		anim+=.001;
		gl::clear(Color(0, 0, 0));
		
		gl::enableAdditiveBlending();
		gl::color(Color::white());
		gl::drawStrokedCube(AxisAlignedBox3f(vec3(0,0,0),vec3(wsz,wsz,wsz)));
		//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		gl::color(Color::gray(.5));
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(.5);
		for(int i = 0; i < 3;i++)
			psys.update();
		auto setColor = [&](Part& part){ return gl::color(hsvToRgb(vec3(fmod(part.index,1), .95,.2))); };
		foreach(auto& part, psys.parts)
		{
			setColor(part);
			glPushMatrix();
			swapYZ();
			if(!keys['0']){
				gl::drawSolidCircle(part.pos.front().vec, 3);
				gl::drawSolidCircle(part.pos.back().vec, 3);
			}
			glPopMatrix();
			glBegin(GL_LINE_STRIP);
			float brightness=.05;
			for(int i = part.pos.size()-1;i>=0;i--) {
				auto pos=part.pos[i];
				if(pos.wrap) { glEnd(); glBegin(GL_LINE_STRIP); continue; }
				gl::vertex(pos.vec.x, length(pos.vel)*100, pos.vec.y);
				auto dist=length(pos.vel); auto step=normalize(pos.vel); auto p = pos.vec;
				for(int i=0;i<dist;i++) {
					p+=step;
					//interact_map.limitGet(p) += pos.vel*10;
				}
			}
			glEnd();
		}
		for(int i=0;i<interact_map.area;i++) interact_map(i)*=.99f;
		//satBlur<vec2, Vec2d>(interact_map, 10);
		gl::color(1,0,0);
		glLineWidth(1);
		if(showInteractMap)
		{
			for(int x = 0; x < wsz; x+=10)
			{
				for(int y = 0; y < wsz; y+=10)
				{
					auto in = interact_map(x, y);
					in = normalize(in) * log(length(in)+1);
					gl::drawLine(vec2(x, y), vec2(x, y) + in);
				}
			}
		}
	}
};

CINDER_APP_BASIC(SApp, RendererGl)
