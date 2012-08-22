/*
	TUIO C++ Demo - part of the reacTIVision project
	http://reactivision.sourceforge.net/

	Copyright (c) 2005-2009 Martin Kaltenbrunner <mkalten@iua.upf.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "TuioDemo.h"
		
void TuioDemo::addTuioObject(TuioObject *tobj) {
	if (verbose)
		std::cout << "add obj " << tobj->getSymbolID() << " (" << tobj->getSessionID() << ") "<< tobj->getX() << " " << tobj->getY() << " " << tobj->getAngle() << std::endl;

}

void TuioDemo::updateTuioObject(TuioObject *tobj) {

	if (verbose) 	
		std::cout << "set obj " << tobj->getSymbolID() << " (" << tobj->getSessionID() << ") "<< tobj->getX() << " " << tobj->getY() << " " << tobj->getAngle() 
		<< " " << tobj->getMotionSpeed() << " " << tobj->getRotationSpeed() << " " << tobj->getMotionAccel() << " " << tobj->getRotationAccel() << std::endl;

}

void TuioDemo::removeTuioObject(TuioObject *tobj) {

	if (verbose)
		std::cout << "del obj " << tobj->getSymbolID() << " (" << tobj->getSessionID() << ")" << std::endl;
}

void TuioDemo::addTuioCursor(TuioCursor *tcur) {
	
	if (verbose) 
		std::cout << "add cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY() << std::endl;

}

void TuioDemo::updateTuioCursor(TuioCursor *tcur) {

	if (verbose) 	
		std::cout << "set cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY() 
					<< " " << tcur->getMotionSpeed() << " " << tcur->getMotionAccel() << " " << std::endl;
}

void TuioDemo::removeTuioCursor(TuioCursor *tcur) {
	
	if (verbose)
		std::cout << "del cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ")" << std::endl;
}

void TuioDemo::refresh(TuioTime frameTime) {
	//drawObjects();
}


void TuioDemo::drawObjects() {
	glClear(GL_COLOR_BUFFER_BIT);
	char id[3];

	// draw the cursors
	std::list<TuioCursor*> cursorList = tuioClient->getTuioCursors();
	tuioClient->lockCursorList();
	for (std::list<TuioCursor*>::iterator iter = cursorList.begin(); iter!=cursorList.end(); iter++) {
		TuioCursor *tuioCursor = (*iter);
		std::list<TuioPoint> path = tuioCursor->getPath();
		if (path.size()>0) {

		TuioPoint last_point = path.front();
		glBegin(GL_LINES);
		glColor3f(0.0, 0.0, 1.0);
		
		for (std::list<TuioPoint>::iterator point = path.begin(); point!=path.end(); point++) {
			glVertex3f(last_point.getX()*width, last_point.getY()*height, 0.0f);
			glVertex3f(point->getX()*width, point->getY()*height, 0.0f);
			last_point.update(point->getX(),point->getY());

		} glEnd();

		// draw the finger tip
		glColor3f(0.75, 0.75, 0.75);
		glPushMatrix();
		glTranslatef(last_point.getX()*width, last_point.getY()*height, 0.0);
		glBegin(GL_TRIANGLE_FAN);
		for(double a = 0.0f; a <= 2*M_PI; a += 0.2f) {
			glVertex2d(cos(a) * height/100.0f, sin(a) * height/100.0f);
		} glEnd();
		glPopMatrix();
		
		glColor3f(0.0, 0.0, 0.0);
		glRasterPos2f(tuioCursor->getScreenX(width),tuioCursor->getScreenY(height));
		sprintf(id,"%d",tuioCursor->getCursorID());
		drawString(id);
		}
	}
	tuioClient->unlockCursorList();

	// draw the objects
	std::list<TuioObject*> objectList = tuioClient->getTuioObjects();
	tuioClient->lockObjectList();
	for (std::list<TuioObject*>::iterator iter = objectList.begin(); iter!=objectList.end(); iter++) {
		TuioObject* tuioObject = (*iter);
		int pos_size = height/20.0f;
		int neg_size = -1*pos_size;
		float xpos  = tuioObject->getX()*width;
		float ypos  = tuioObject->getY()*height;
		float angle = tuioObject->getAngle()/M_PI*180;
		
		glColor3f(0.0, 0.0, 0.0);
		glPushMatrix();
		glTranslatef(xpos, ypos, 0.0);
		glRotatef(angle, 0.0, 0.0, 1.0);
		glBegin(GL_QUADS);
			glVertex2f(neg_size, neg_size);
			glVertex2f(neg_size, pos_size);
			glVertex2f(pos_size, pos_size);
			glVertex2f(pos_size, neg_size);
		glEnd();
		glPopMatrix();

		glColor3f(1.0, 1.0, 1.0);
		glRasterPos2f(xpos,ypos+5);
		sprintf(id,"%d",tuioObject->getSymbolID());
		drawString(id);
	}
	tuioClient->unlockObjectList();

	SDL_GL_SwapBuffers();
}

void TuioDemo::drawString(char *str) {
	if (str && strlen(str)) {
		while (*str) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *str);
			str++;
		}
	}
}

void TuioDemo::toggleFullscreen() {

        if( !window || SDL_WM_ToggleFullScreen(window)!=1 ) {
                std::cout << "Unable to toggle fullscreen: " << SDL_GetError() << std::endl;
        } else {
                fullscreen = !fullscreen;
                SDL_ShowCursor(!fullscreen);
        }
}

void TuioDemo::processEvents()
{
    SDL_Event event;

    while( SDL_PollEvent( &event ) ) {

        switch( event.type ) {
		case SDL_KEYDOWN:
			if( event.key.keysym.sym == SDLK_ESCAPE ){
				running = false;
				SDL_ShowCursor(true);
				SDL_Quit();
			} else if( event.key.keysym.sym == SDLK_F1 ){
				toggleFullscreen();
			} else if( event.key.keysym.sym == SDLK_v ){
				verbose = !verbose;	
			} 

			break;
		case SDL_QUIT:
			running = false;
			SDL_ShowCursor(true);
			SDL_Quit();
			break;
        }
    }
}

TuioDemo::TuioDemo(int port) {


	verbose = false;
	fullscreen = false;
	window_width = 640;
	window_height = 480;
	screen_width = 1024;
	screen_height = 768;

	tuioClient = new TuioClient(port);
	tuioClient->addTuioListener(this);
	tuioClient->connect();

	if (!tuioClient->isConnected()) {
		SDL_Quit();
		exit(1);
	}
	
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		std::cerr << "SDL could not be initialized: " <<  SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	
	int videoFlags = SDL_OPENGL | SDL_DOUBLEBUF;
	if( fullscreen ) {
		videoFlags |= SDL_FULLSCREEN;
		SDL_ShowCursor(false);
		width = screen_width;
		height = screen_height;
	} else {
		width = window_width;
		height = window_height;
	}
	
	window = SDL_SetVideoMode(width, height, 32, videoFlags);
	if ( window == NULL ) {
		std::cerr << "Could not open window: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();
	gluOrtho2D(0, (float)width,  (float)height, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	SDL_WM_SetCaption("TuioDemo", NULL);
}

void TuioDemo::run() {
	running=true;
	while (running) {
		drawObjects();
		processEvents();
		SDL_Delay(40);
	} 
}

int main(int argc, char* argv[])
{
	if( argc >= 2 && strcmp( argv[1], "-h" ) == 0 ){
        	std::cout << "usage: TuioDemo [port]\n";
        	return 0;
	}

	int port = 3333;
	if( argc >= 2 ) port = atoi( argv[1] );

#ifndef __MACOSX__
	glutInit(&argc,argv);
#endif
	
	TuioDemo *app = new TuioDemo(port);
	app->run();
	delete(app);

	return 0;
}


