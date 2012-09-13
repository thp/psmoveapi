/* ========================================================================
 *
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Released under the same terms as the OpenTracker library itself.
 *
 * Based on example code from:
 * Copyright (c) 2006,
 * Institute for Computer Graphics and Vision
 * Graz University of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the Graz University of Technology nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================
 * PROJECT: OpenTracker
 * ======================================================================== */

// this will remove the warning 4786
#include <OpenTracker/tool/disable4786.h>

#include <OpenTracker/input/PSMoveModule.h>

#ifdef USE_PSMOVE

#include <iostream>
#include <string>

#include <OpenTracker/input/PSMoveSource.h>

namespace ot {

    // XXX: If we don't use PSMoveConfig, do we need to use that?
    OT_MODULE_REGISTER_FUNC(PSMoveModule) {
        OT_MODULE_REGISTRATION_DEFAULT(PSMoveModule, "PSMoveConfig");
    }

    PSMoveModule::PSMoveModule()
        : Module(),
          NodeFactory(),
          tracker(psmove_tracker_new())
    {
        logPrintI("PSMoveModule\n");
    }

    PSMoveModule::~PSMoveModule()
    {
        psmove_tracker_free(tracker);
        logPrintI("~PSMoveModule\n");
    }

    void
    PSMoveModule::init(StringTable &attributes, ConfigNode *localTree)
    {
        logPrintI("PSMoveModule::init\n");
        Module::init(attributes, localTree);
    }

    Node *
    PSMoveModule::createNode(const std::string &name,
            const StringTable &attributes)
    {
        if (name.compare("PSMoveSource") == 0) {
            logPrintI("PSMoveModule::createNode\n");

            int count, controller;
            int connected = psmove_count_connected();

            if (connected == 0) {
                logPrintE("No PS Move controllers connected.\n");
                return NULL;
            }

            count = attributes.get("controller", &controller);

            if (!count) {
                // Invalid or empty attribute?
                controller = 0;
            } else if (controller < 0 || controller >= connected) {
                logPrintE("Invalid controller index (%d connected)\n",
                        connected);
                return NULL;
            }

            PSMoveSource *source = new PSMoveSource(tracker, controller);
            nodes.push_back(source);
            logPrintI("Built PSMoveSource node for controller %d n",
                    controller);
            initialized = 1;
            return source;
        }

        return NULL;
    }

    void
    PSMoveModule::start()
    {
        // Nothing to be done here (for now)
    }

    void
    PSMoveModule::close()
    {
        initialized = 0;
        // FIXME: delete all sources here, so psmove_disconnect() gets called?
        nodes.clear();
    }

    void
    PSMoveModule::pushEvent()
    {
        if (isInitialized()) {
            // Update the CV tracking for all controllers at once
            psmove_tracker_update_image(tracker);
            psmove_tracker_update(tracker, NULL);

            NodeVector::iterator it;
            for (it=nodes.begin(); it != nodes.end(); ++it) {
                PSMoveSource *source = (PSMoveSource*)((Node*)*it);

                Event &event = source->getEvent();
                event.setConfidence(1.f);
                event.timeStamp();
                source->updateObservers(event);
            }
        }
    }

} /* namespace ot */

#endif /* USE_PSMOVE */
