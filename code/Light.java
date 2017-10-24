package library;

import javax.media.opengl.*;
import javax.media.opengl.glu.*;
import java.nio.FloatBuffer;

public class Light extends Object3D
{
    public class GLLight
    {
        public Point pos = new Point();
        public Normal dir = new Normal();
        public RGBA diffuse = new RGBA();
        public RGBA specular = new RGBA();
        public RGBA ambient = new RGBA();
        public float range;
        public float falloff;
        public float attenuation;
        public float theta;
        public float phi;

        public GLLight()
        {
            range = 0.0f;
            falloff = 0.0f;
            attenuation = 0.0f;
            theta = 0.0f;
            phi = 0.0f;
        }

        public void equ(GLLight obj)
        {
            pos.equ(obj.pos);
            dir.equ(obj.dir);
            diffuse.equ(obj.diffuse);
            specular.equ(obj.specular);
            ambient.equ(obj.ambient);

            range = obj.range;
            falloff = obj.falloff;
            attenuation = obj.attenuation;
            theta = obj.theta;
            phi = obj.phi;
        }
    }

    // OpenGL light data
    public GLLight light = new GLLight();
    public GLLight trans_light = new GLLight();

    // Light type
    public int type;

    // Handle to light
    public int handle;

    // is light on or off
    public boolean enabled;

    // Does this light cast a shadow
    public boolean castShadow;

    public static int POINT_LIGHT = 0;
    public static int DIRECTIONAL_LIGHT = 1;
    public static int SPOT_LIGHT = 2;

    public int compareTo(Object3D obj)
    {
        return 0;
    }

    /************************************************************************
    *    desc:  Constructer
    ************************************************************************/
    public Light()
    {
        handle = 0;
        enabled = false;
        castShadow = false;

    }   // Constructer

    /************************************************************************
    *    desc:  Transform the light to this sprite's model view
    *
    *    param: GL gl - OpenGL object
    *           Matrix matrix - passed in matrix
    ************************************************************************/
    public void transform( GL gl, Matrix matrix )
    {
        if( type == POINT_LIGHT )
        {
            transformForPointLight( gl, matrix );
        }
        else if( type == DIRECTIONAL_LIGHT )
        {
           transformForDirectionalLight( gl, matrix );
        }
        else if( type == SPOT_LIGHT )
        {
            transformForSpotLight( gl, matrix );
        }
    }	// Transform


    /************************************************************************
    *    desc:  Transform to model view for point light
    *
    *    param: GL gl - OpenGL object
    *           Matrix matrix - passed in matrix
    ************************************************************************/
    public void transformForPointLight( GL gl, Matrix matrix )
    {
        // Transform the light position
        matrix.transform( trans_light.pos, light.pos );

        FloatBuffer fb;
        fb = FloatBuffer.wrap(new float[] { trans_light.pos.x, trans_light.pos.y, trans_light.pos.z, 1.0f });
        gl.glLightfv( handle, gl.GL_POSITION, fb );
    }	// TransformForPointLight


    /************************************************************************
    *    desc:  Transform to model view for directional light
    *
    *    param: GL gl - OpenGL object
    *           Matrix matrix - passed in matrix
    ************************************************************************/
    public void transformForDirectionalLight( GL gl, Matrix matrix )
    {
        // Transform the light position
        matrix.transform( trans_light.dir, light.dir );

        FloatBuffer fb;
        fb = FloatBuffer.wrap(new float[] { trans_light.dir.x, trans_light.dir.y, trans_light.dir.z, 0.0f });
        gl.glLightfv( handle, gl.GL_POSITION, fb );

    }	// TransformForDirectionalLight


    /************************************************************************
    *    desc:  Transform to model view for spot light
    *
    *    NOTE: A spot light is a point and directional light so all we need
    *          to do is reuse our functions
    *
    *    param: GL gl - OpenGL object
    *           Matrix matrix - passed in matrix
    ************************************************************************/
    public void transformForSpotLight( GL gl, Matrix matrix )
    {
        transformForPointLight( gl, matrix );

        // Transform the light position
        matrix.transform( trans_light.dir, light.dir );

        FloatBuffer fb;
        fb = FloatBuffer.wrap(new float[] { trans_light.dir.x, trans_light.dir.y, trans_light.dir.z });
        //spot direction
        gl.glLightfv(handle, gl.GL_SPOT_DIRECTION, fb);

    }	// TransformForSpotLight


    /************************************************************************
    *    desc:  Set the object position
    *
    *    param: CPoint & position - pos to set
    ************************************************************************/
    public void setPos( Point position )
    {
        // Call the parent
        super.setPos( position );

        trans_light.pos.x = light.pos.x = getPos().x;
        trans_light.pos.y = light.pos.y = getPos().y;
        trans_light.pos.z = light.pos.z = getPos().z;
    }   // SetPos


    /************************************************************************
    *    desc:  Inc the object position
    *
    *    param: Point position - pos to inc
    ************************************************************************/
    public void incPos( Point position )
    {
        // Call the parent
        super.incPos( position );

        trans_light.pos.x = light.pos.x = getPos().x;
        trans_light.pos.y = light.pos.y = getPos().y;
        trans_light.pos.z = light.pos.z = getPos().z;
    }   // IncPos

    /************************************************************************
    *    desc:  Get the type of light
    *
    *    return: int - Get the light type
    ************************************************************************/
    public int getType()
    {
        return type;
    }

    /************************************************************************
    *    desc:  Get the lights direction
    *
    *    return: Normal - Normalized direction
    ************************************************************************/
    public Normal getDir()
    {
        Normal dir = new Normal();
        dir.equ( light.dir.x, light.dir.y, light.dir.z );

        return dir;
    }

    /************************************************************************
    *    desc:  Transform to model view for point light
    *
    *    param: bool enable - on or off
    *           GL gl - OpenGL object
    ************************************************************************/
    public void enable( boolean enable, GL gl )
    {
        if(enable == true)
        {
            gl.glEnable( handle );
        }
        else
        {
            gl.glDisable( handle );
        }

        enabled = enable;
    }
}
