## Create the Interface library.
ADD_LIBRARY(Manta_Interface
	      CameraPath.cc
	      CameraPath.h
        AccelerationStructure.h
        AmbientLight.cc
        AmbientLight.h
        Background.cc
        Background.h
        Camera.cc
        Camera.h
        Clonable.h
        Clonable.cc
        Context.h
        Context.cc
        Fragment.h
        IdleMode.cc
        IdleMode.h
        Image.cc
        Image.h
        ImageDisplay.cc
        ImageDisplay.h
        ImageTraverser.cc
        ImageTraverser.h
        InterfaceRTTI.cc
        InterfaceRTTI.h
        Interpolable.cc
        Interpolable.h
        Light.cc
        Light.h
        LightSet.cc
        LightSet.h
        LoadBalancer.cc
        LoadBalancer.h
        MantaInterface.cc
        MantaInterface.h
        Material.cc
        Material.h
        Object.cc
        Object.h
        PixelSampler.cc
        PixelSampler.h
        Primitive.cc
        Primitive.h
        RandomNumberGenerator.cc
        RandomNumberGenerator.h
        RayPacket.cc
        RayPacket.h
        RenderParameters.h
        Renderer.cc
        Renderer.h
        SampleGenerator.h
        Scene.cc
        Scene.h
        SetupCallback.cc
        RenderParameters.cc
        SetupCallback.h
        ShadowAlgorithm.cc
        ShadowAlgorithm.h
	Task.h
	Task.cc
	TaskQueue.h
	TaskQueue.cc
        TexCoordMapper.cc
        TexCoordMapper.h
        Texture.h
        Transaction.cc
        Transaction.h
        UserInterface.cc
        UserInterface.h
        XWindow.h
        )

TARGET_LINK_LIBRARIES(Manta_Interface Manta_Core)
