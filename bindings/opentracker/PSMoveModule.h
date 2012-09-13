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

/**
 * @page module_ref Module Reference
 * @section psmovemodule PSMoveModule
 *
 * The PSMoveModule provides and drives @ref psmovesource nodes. It implements
 * a driver for the Playstation Move Motion Controller using the PS Move API.
 */

#ifndef _PSMOVEMODULE_H
#define _PSMOVEMODULE_H

#include "../OpenTracker.h"

#include "psmove.h"
#include "psmove_tracker.h"

#ifdef USE_PSMOVE

/**
 * The module and factory to drive the PSMoveSource nodes. It constructs
 * PSMoveSource nodes via the NodeFactory interface and pushes events into
 * the tracker tree according to the nodes configuration.
 *
 * Based on the Hydra module by Hannes Kaufmann, Istvan Barakonyi, Mathis Csisinko
 *
 * @author Thomas Perl
 * @ingroup input
 */

namespace ot {
    class OPENTRACKER_API PSMoveModule : public Module, public NodeFactory {
        protected:
            NodeVector nodes;
            PSMoveTracker *tracker;

        public:

            PSMoveModule();
            ~PSMoveModule();

            /**
             * Initializes the PS Move module
             *
             * @param attributes Attribute values for the module
             * @param localTree Pointer to root of configuration tree
             */
            void
            init(StringTable &attributes, ConfigNode *localTree);

            /**
             * Constructs a new Node, representing a PS Move Motion Controller
             *
             * @param name Name of the element to create
             * @param attributes Attribute values for the element
             * @return Pointer to new Node or NULL
             **/
            Node *
            createNode(const std::string &name, const StringTable &attributes);

            void start();
            void close();
            void pushEvent();
        };

	OT_MODULE(PSMoveModule);
} /* namespace ot */

#endif /* USE_PSMOVE */

#endif /* _PSMOVEMODULE_H */
