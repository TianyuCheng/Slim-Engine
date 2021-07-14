#ifndef SLIM_UTILITY_COMPONENT_H
#define SLIM_UTILITY_COMPONENT_H

namespace slim {

    class Component {
    public:
        virtual void OnStart();
        virtual void OnUpdate();
    }; // end of scene graph class

} // end of namespace slim

#endif // end of SLIM_UTILITY_SCENEGRAPH_H
