#include "precompiled.h"
#include "ciextra.h"
#include <cinder/gl/Fbo.h>
#include <cinder/Camera.h>
#include "util.h"
#include "sat.h"
Perlin perlin(5);
const int wsz=600;
Vec2f perlin_lookup[wsz][wsz];
float anim=0;
int splitAge=1000;
Array2D<Vec2f> interact_map(wsz,wsz);
Area world(0,0, wsz,wsz);

struct Part{
	float index;int age;
	bool slowlyDie;
	struct Pos
	{
		Vec2f vec;
		Vec2f vel;
		bool wrap;
		Pos(Vec2f vec, bool wrap){ this->vec=vec; this->wrap=wrap;vel=Vec2f::zero();}
	};
	deque<Pos> pos;
	Part(Vec2f pos,float index){
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
		auto d2D = Vec2f(pos.back().vec.x/wsz, pos.back().vec.y/wsz)*2;
		float dx=sin(anim)*perlin.fBm(d2D.x, d2D.y, 0+anim+index);
		float dy=cos(anim)*perlin.fBm(d2D.x, d2D.y, .5+anim+index);
		auto d = Vec2f(dx, dy)*10;
		//d += interact_map.limitGet(pos.back().vec);
		pos.back().vel+=d;//d.normalized()*max(10.f,d.length());
		pos.back().vel*=.999;
		index+=std::log(1+pos.back().vel.length())*.00001;
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
				newPart.pos.front().vel.rotate(-1);
				part.pos.front().vel.rotate(1);
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
bool showInteractMap; bool captureMouse=true;Vec3f worldUp(0,1,0);
struct SApp : AppBasic {
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
		camPos=Vec3f(0,0, 0);
		keys.resize(256);
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
				psys.parts.push_back(Part(Vec2f(0,i),pindex));
				pindex+=pindexstep;
				psys.parts.push_back(Part(Vec2f(getWindowWidth(),i),pindex));
				pindex+=pindexstep;
			}
			if(i<getWindowWidth()){
				psys.parts.push_back(Part(Vec2f(i,0),pindex));
				pindex+=pindexstep;
				psys.parts.push_back(Part(Vec2f(i,getWindowHeight()),pindex));
				pindex+=pindexstep;
			}
		}
	}
	void mouseDown(MouseEvent e)
	{
	}
	float updownRot, leftrightRot; Vec3f camPos; float yvel; Vec2i oldMPos;
	void mouseMove(MouseEvent e)
	{
		if(e.getPos()==oldMPos)return;//recursion
		
		Quatf o = cam.getOrientation();
		leftrightRot-=e.getX()-oldMPos.x;
		updownRot+=e.getY()-oldMPos.y;
		
		if(captureMouse)
		{
			centerMouse();
		}else
			oldMPos=e.getPos();
		//cam.setOrientation(Quatf(0, leftrightRot/1000.0, 0)*Quatf(updownRot/1000.0, 0, 0));
		//cam.setWorldUp(Vec3f(0, 0, 1));
	}
	void centerMouse() { auto newCursorPos = getWindowPos() + Vec2i(wsz,wsz)/2;SetCursorPos(newCursorPos.x, newCursorPos.y); oldMPos=newCursorPos-getWindowPos(); }
	vector<bool> keys;
	void keyDown(KeyEvent e)
	{
		keys[e.getChar()]=true;
		if(e.getChar()=='i')
			showInteractMap = !showInteractMap;
		if(e.getCode()==KeyEvent::KEY_ESCAPE)
			quit();
		if(e.getChar()=='c')
			captureMouse = !captureMouse;
		if(e.getChar()==' ')
			yvel=10;
	}
	void keyUp(KeyEvent e)
	{
		keys[e.getChar()]=false;
	}
	Matrix33f getCameraRotation() {
		//return Matrix33f::createRotation(Vec3f(
		return Matrix33f::createRotation(Vec3f(updownRot/1000.0,leftrightRot/1000.0,0));
	}

	void updateCam()
	{
		//Matrix33f cameraRotation = Matrix.CreateRotationX(leftrightRot) * Matrix.CreateRotationY(updownRot);
		auto cameraRotation = this->getCameraRotation();
		Vec3f cameraOriginalTarget = Vec3f(0, 0, 1);
		Vec3f cameraRotatedTarget = cameraRotation.transformVec(cameraOriginalTarget);
		Vec3f cameraFinalTarget = camPos + cameraRotatedTarget;

		Vec3f cameraOriginalUpVector = worldUp;
		Vec3f cameraRotatedUpVector = cameraRotation.transformVec(cameraOriginalUpVector);
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
				Vec3f(1, 0, 0),
				Vec3f(0, 0, 1),
				Vec3f(0, 1, 0)));
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
		const int camSpeed=10;
		Vec3f moveDir = Vec3f(cam.getViewDirection().x, 0, cam.getViewDirection().z).normalized() * camSpeed;
		if(keys['w'])
			camPos+=moveDir;
		if(keys['s'])
			camPos-=moveDir;
		if(keys['a'])
			camPos-=moveDir.cross(worldUp);
		if(keys['d'])
			camPos+=moveDir.cross(worldUp);
		const float fff=5;
		if(keys['i'])updownRot+=fff;
		if(keys['k'])updownRot-=fff;
		if(keys['j'])leftrightRot+=fff;
		if(keys['l'])leftrightRot-=fff;
		

		if(captureMouse)
			centerMouse();

		//cam.setCenterOfInterestPoint(Vec3f(getMousePos().x, wsz-getMousePos().y, 0));
		cam.setFarClip(10000);
		gl::setMatrices(cam);

		anim+=.001;
		gl::clear(Color(0, 0, 0));
		
		gl::enableAdditiveBlending();
		gl::color(Color::white());
		gl::drawStrokedCube(AxisAlignedBox3f(Vec3f(0,0,0),Vec3f(wsz,wsz,wsz)));
		//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		gl::color(Color::gray(.5));
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(.5);
		for(int i = 0; i < 3;i++)
			psys.update();
		auto setColor = [&](Part& part){ return gl::color(hsvToRGB(Vec3f(fmod(part.index,1), .95,.2))); };
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
				gl::vertex(pos.vec.x, pos.vel.length()*100, pos.vec.y);
				auto dist=pos.vel.length(); auto step=pos.vel.normalized(); auto p = pos.vec;
				for(int i=0;i<dist;i++) {
					p+=step;
					//interact_map.limitGet(p) += pos.vel*10;
				}
			}
			glEnd();
		}
		for(int i=0;i<interact_map.area;i++) interact_map(i)*=.99f;
		//satBlur<Vec2f, Vec2d>(interact_map, 10);
		gl::color(1,0,0);
		glLineWidth(1);
		if(showInteractMap)
		{
			for(int x = 0; x < wsz; x+=10)
			{
				for(int y = 0; y < wsz; y+=10)
				{
					auto in = interact_map(x, y);
					in = in.normalized() * log(in.length()+1);
					gl::drawLine(Vec2f(x, y), Vec2f(x, y) + in);
				}
			}
		}
	}
};

CINDER_APP_BASIC(SApp, RendererGl)
