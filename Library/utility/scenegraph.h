#ifndef SLIM_UTILITY_SCENEGRAPH_H
#define SLIM_UTILITY_SCENEGRAPH_H

#include "utility/interface.h"
#include "utility/transform.h"

namespace slim {

    class SceneGraph final {
    public:
        class Node;
        friend class Node;

        class Node {
        public:
            friend class SceneGraph;
            explicit Node();
            virtual ~Node();
            void AddChild(Node *node);
        }; // end of scene node class

        explicit SceneGraph();
        virtual ~SceneGraph();

        Node* GetRoot();

    }; // end of scene graph class

} // end of namespace slim

#endif // end of SLIM_UTILITY_SCENEGRAPH_H
