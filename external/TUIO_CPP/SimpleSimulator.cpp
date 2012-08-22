/*
	TUIO C++ Server Demo - part of the reacTIVision project
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

#include "SimpleSimulator.h"
void SimpleSimulator::drawFrame() {
	
	if(!running) return;
	glClear(GL_COLOR_BUFFER_BIT);
	char id[3];
	
	// draw the cursors
	std::list<TuioCursor*> cursorList = tuioServer->getTuioCursors();
	for (std::list<TuioCursor*>::iterator tuioCursor = cursorList.begin(); tuioCursor!=cursorList.end(); tuioCursor++) {
		std::list<TuioPoint> path = (*tuioCursor)->getPath();
		if (path.size()>0) {
			
			TuioPoint last_point = path.front();
			glBegin(GL_LINES);
			glColor3f(0.0, 0.0, 1.0);
			
			for (std::list<TuioPoint>::iterator point = path.begin(); point!=path.end(); point++) {
				glVertex3f(last_point.getX()*width, last_point.getY()*height, 0.0f);
				glVertex3f(point->getX()*width, point->getY()*height, 0.0f);
				last_point.update(point->getX(),point->getY());
			}
			glEnd();
			
			// draw the finger tip
			glColor3f(0.75, 0.75, 0.75);
			glPushMatrix();
			glTranslatef(last_point.getX()*width, last_point.getY()*height, 0.0);
			glBegin(GL_TRIANGLE_FAN);
			for(double a = 0.0f; a <= 2*M_PI; a += 0.2f) {
				glVertex2d(cos(a) * height/100.0f, sin(a) * height/100.0f);
			}
			glEnd();
			glPopMatrix();
			
			glColor3f(0.0, 0.0, 0.0);
			glRasterPos2f((*tuioCursor)->getScreenX(width),(*tuioCursor)->getScreenY(height));
			sprintf(id,"%d",(*tuioCursor)->getCursorID());
			drawString(id);
		}
	}
	
	SDL_GL_SwapBuffers();
}

void SimpleSimulator::drawString(char *str) {
	if (str && strlen(str)) {
		while (*str) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *str);
			str++;
		}
	}
}

void SimpleSimulator::toggleFullscreen() {

        if( !window || SDL_WM_ToggleFullScreen(window)!=1 ) {
                std::cout << "Unable to toggle fullscreen: " << SDL_GetError() << std::endl;
        } else {
                fullscreen = !fullscreen;
                SDL_ShowCursor(!fullscreen);
        }
}

void SimpleSimulator::processEvents()
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
				tuioServer->setVerbose(verbose);
			} 
			break;
				
		case SDL_MOUSEMOTION:
			if (event.button.button & SDL_BUTTON(1)) mouseDragged((float)event.button.x/width, (float)event.button.y/height);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button & SDL_BUTTON(1)) mousePressed((float)event.button.x/width, (float)event.button.y/height);
			break;
		case SDL_MOUSEBUTTONUP:
			if (event.button.button & SDL_BUTTON(1)) mouseReleased((float)event.button.x/width, (float)event.button.y/height);
			break;
		case SDL_QUIT:
			running = false;
			SDL_ShowCursor(true);
			SDL_Quit();
			break;
        }
    }
}

void SimpleSimulator::mousePressed(float x, float y) {
	//printf("clicked %i %i\n",x,y);
	
	TuioCursor *match = NULL;
	for (std::list<TuioCursor*>::iterator tuioCursor = stickyCursorList.begin(); tuioCursor!=stickyCursorList.end(); tuioCursor++) {
		if ((*tuioCursor)->getDistance(x,y) < (5.0f/width)) {
			match = (*tuioCursor);
			break;
		}
	}
	
	Uint8 *keystate = SDL_GetKeyState(NULL);
	if ((keystate[SDLK_LSHIFT]) || (keystate[SDLK_RSHIFT]))  {
			
		if (match!=NULL) {
			stickyCursorList.remove(match);
			tuioServer->removeTuioCursor(match);
			cursor = NULL;
		} else {
			cursor = tuioServer->addTuioCursor(x,y);
			stickyCursorList.push_back(cursor);
		}
	} else {
		if (match!=NULL) {
			cursor=match;
		} else {
			cursor = tuioServer->addTuioCursor(x,y);
		}
	}	
}

void SimpleSimulator::mouseDragged(float x, float y) {
	//printf("dragged %i %i\n",x,y);
	if (cursor==NULL) return;
	if (cursor->getTuioTime()==currentTime) return;
	tuioServer->updateTuioCursor(cursor,x,y);
}

void SimpleSimulator::mouseReleased(float x, float y) {
	//printf("released\n");
	if (cursor==NULL) return;
	
	TuioCursor *match = NULL;
	std::list<TuioCursor*> cursorList = tuioServer->getTuioCursors();
	for (std::list<TuioCursor*>::iterator tuioCursor = stickyCursorList.begin(); tuioCursor!=stickyCursorList.end(); tuioCursor++) {
		if ((*tuioCursor)->getDistance(x,y) < (5.0f/width)) {
			match = (*tuioCursor);
			break;
		}
	}
	
	if (match==NULL) {
		tuioServer->removeTuioCursor(cursor);
	}
	cursor = NULL;
}

SimpleSimulator::SimpleSimulator(const char* host, int port) {

	verbose = false;
	fullscreen = false;
	window_width = 640;
	window_height = 480;
	screen_width = 1024;
	screen_height = 768;
	
	cursor = NULL;
	if ((strcmp(host,"default")==0) && (port==0)) tuioServer = new TuioServer();
	else tuioServer = new TuioServer(host, port);
	currentTime = TuioTime::getSessionTime();
	
	//tuioServer->enablePeriodicMessages();

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
	SDL_WM_SetCaption("SimpleSimulator", NULL);
}

void SimpleSimulator::run() {
	running=true;
	while (running) {
		currentTime = TuioTime::getSessionTime();
		tuioServer->initFrame(currentTime);
		processEvents();
		tuioServer->stopUntouchedMovingCursors();
		tuioServer->commitFrame();
		drawFrame();
		SDL_Delay(20);
	} 
}

int main(int argc, char* argv[])
{
	if (( argc != 1) && ( argc != 3)) {
        	std::cout << "usage: SimpleSimulator [host] [port]\n";
        	return 0;
	}

#ifndef __MACOSX__
	glutInit(&argc,argv);
#endif
	
	SimpleSimulator *app;
	if( argc == 3 ) {
		app = new SimpleSimulator(argv[1],atoi(argv[2]));
	} else app = new SimpleSimulator("default",0);
	
	app->run();
	delete(app);

	return 0;
}


