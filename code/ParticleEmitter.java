package library;

import library.*;
import java.util.*;
import javax.media.opengl.*;
import java.lang.Math;

public class ParticleEmitter extends Particles
{
    //Particles testPart;

    private Point pos = new Point();
    private boolean visible = true;

    //The array of sprites for the effect. One sprite per particle
    private GLSprite[] particlesB;
    //The total number of particles that will be used for the effect
    private final int MAX_PARTICLES;

    //An x and a y variable is required to calculate each sprite's individual
    //positions
    private float[] partX;
    private float[] partY;
    private float[] partZ;
    //private float[] partH;

    //Three different variables which will get their numbers from the generator
    private Random generator = new Random(System.currentTimeMillis());
    private float randomFloat;
    private float randomAngle1;
    //randomAngle2 is used in the calulations of the position of a particle
    //so each particle will require their own value
    private float[] randomAngle2;

    //private float elapsedTime;

    private float maxMinXEmitterSize;
    private float maxMinYEmitterSize;
    private float maxMinZEmitterSize;

    public int compareTo(Object3D obj)
    {
        return 0;
    }

     public ParticleEmitter(int maxParticles, float radius, String file)
     {
         MAX_PARTICLES = maxParticles;
         init(maxParticles, radius, file);

          partX = new float[MAX_PARTICLES];
          partY = new float[MAX_PARTICLES];
          partZ = new float[MAX_PARTICLES];
          //partH = new float[MAX_PARTICLES];
          randomAngle2 = new float[MAX_PARTICLES];

          for(int i = 0; i < MAX_PARTICLES; i++)
          {
              partX[i] = 0.0f;
              partY[i] = 0.0f;
              partZ[i] = 0.0f;
          }
     }

    /************************************************************************
    *    desc:  ParticleEmitter Constructor
    *
    *    param: String - file (file name), and the maximum particles that
    *           will be used in the effect
    ************************************************************************/
    public ParticleEmitter(String file, int maxParticles)
    {
        //Sets the maximum particles used in the effect. Key for many for loops required
        MAX_PARTICLES = maxParticles;
        particlesB = new GLSprite[MAX_PARTICLES];
        partX = new float[MAX_PARTICLES];
        partY = new float[MAX_PARTICLES];
        partZ = new float[MAX_PARTICLES];
        //partH = new float[MAX_PARTICLES];
        randomAngle2 = new float[MAX_PARTICLES];

        for(int i = 0; i < MAX_PARTICLES; i++)
        {
            particlesB[i] = new GLSprite();
            particlesB[i].init(file);
            particlesB[i].visible(false);
            partX[i] = 0.0f;
            partY[i] = 0.0f;
            partZ[i] = 0.0f;
        }
    }

    /************************************************************************
    *    desc:  Sets the dimensions of the emitter (the space in which
    *           particles will spawn). So if the x dimension is set to 20,
    *           the length of the emitter on the x dimension will be 40
    *           (from 20 to -20)
    *
    *    param: float - x, y, z
    ************************************************************************/
    public void setEmitterDimensions(float x, float y, float z)
    {
        maxMinXEmitterSize = x;
        maxMinYEmitterSize = y;
        maxMinZEmitterSize = z;
    }

    /************************************************************************
    *    desc:  Sets the position of the emitter as well as the initial position
    *           of the particles in the effect
    *
    *    param: float x, float y, float z for setting the position
    ************************************************************************/
    public void setEmitterAndParticlePos(float x, float y, float z)
    {
        //Sets the position of the emitter
        pos.equ(x, y, z);

        //Sets the positions of the particles
        for(int i = 0; i < MAX_PARTICLES; i++)
        {
            partX[i] = x;
            partY[i] = y;
            partZ[i] = z;
            particles.get(i).setPos(x, y, z);
            //particlesB[i].setPreTransRot(0, 180, 0);
        }
    }

    /************************************************************************
    *    desc:  Gets the position of the emitter
    ************************************************************************/
    public Point getEmitterPos()
    {
        return pos;
    }

    public void billBoardParticles(float x, float y)
    {
        for(int i = 0; i < particlesB.length; i++)
        {            
            particlesB[i].incPreTransRot(-y, 0, 0);
            particlesB[i].incPreTransRot(0, -x, 0);
        }
    }

    /************************************************************************
    *    desc:  Inc the sprite position
    *
    *    param: Point position - pos to inc
    ************************************************************************/
    public void incEmitterOnlyPos( Point position )
    {
        pos.add(position);
    }

    public void incEmitterOnlyPos( float x, float y, float z )
    {
        pos.add(x, y, z);
    }

    /************************************************************************
    *    desc:  A method created to test out how other functions will be made
    *
    *    param: float - time (elapsed time)
    ************************************************************************/
    public void tornadoEffect(float xRot, float yRot, float zRot)
    {
        float elapsedTime = ElapsedTime.getInstance().getElapsedTime();
        
        for(int i = 0; i < MAX_PARTICLES; i++)
        {
            //Get a random number to see if a particle will be made visible if not already
            randomFloat = generator.nextFloat() * 360;

            if(particlesB[i].isVisible() || randomFloat < 1)
            {
                //If the particle is not visible, give it this random angle
                if(!particlesB[i].isVisible())
                {
                    //Random angle generated in order to randomize which way the particle will fly
                    randomAngle1 = generator.nextFloat() * 360;

                    //partX[]
                    particlesB[i].setPostTransRot(0,  randomAngle1, 0);
                    randomAngle2[i] = generator.nextFloat() * 0.1f + .1f;
                }
                else
                {
                    //Equations that map out the path of the particle on the xy plane
                    partX[i] = partX[i] + (elapsedTime * 0.01f);
                    //System.out.println(partX[i] * (float)Math.cos(randomAngle1));
                    partY[i] = randomAngle2[i] * (float)(Math.pow((partX[i]), 2)) + pos.y;
                }
                               
                //Rotate the particles around the center of the emitter for added awesomeness
                //particles[i].incPostTransRot(0, elapsedTime * 0.5f, 0);
                //Set the position of the individual particle
                particlesB[i].setPos(partX[i], partY[i], 0);


                //If the particle reaches a certain height, it becomes invisible and is reset
                //to the emitter's position.
                if(particlesB[i].getPos().y <= 50 + pos.y)
                {
                    particlesB[i].visible(true);
                }
                else
                {
                    //If the particle surpasses the point of death, it will be made
                    //invisible and have it's position reset to that of the emitter
                    particlesB[i].visible(false);
                    partX[i] = pos.x;
                    partY[i] = pos.y;
                    randomAngle2[i] = 0.0f;
                }
            }
            particlesB[i].incPreTransRot(-xRot, -yRot, -zRot);
        }
    }

    public void snowEffect()
    {
        float elapsedTime = ElapsedTime.getInstance().getElapsedTime();

        for(int i = 0; i < MAX_PARTICLES; i++)
        {
            //Get a random number to see if a particle will be made visible if not already
            randomFloat = generator.nextFloat() * 360;

            if(particlesB[i].isVisible() || randomFloat < 2)
            {
                //If the particle is not visible, give it this random position
                if(!particlesB[i].isVisible())
                {
                    randomFloat = generator.nextFloat() * 2 * maxMinXEmitterSize;
                    partX[i] = randomFloat - maxMinXEmitterSize + pos.x;
                    randomFloat = generator.nextFloat() * 2 * maxMinXEmitterSize;
                    partZ[i] = randomFloat - maxMinZEmitterSize + pos.z;

                    particlesB[i].setPos(partX[i], pos.y, partZ[i]);
                }
                else
                {
                    //Equations that map out the path of the particle
                    particlesB[i].incPos(elapsedTime * -0.01f, elapsedTime * -0.05f, 0);
                }

                //If the particle reaches a certain height, it becomes invisible and is reset
                //to the emitter's position.
                if(particlesB[i].getPos().y > -100)
                {
                    particlesB[i].visible(true);
                }
                else
                {
                    //If the particle surpasses the point of death, it will be made
                    //invisible and have it's position reset to that of the emitter
                    particlesB[i].visible(false);
                    particlesB[i].setPos(pos.x, pos.y, pos.z);
                }
            }
        }
    }

    public void test()
    {
        for(int i = 0; i < particles.size(); i++)
        {
            particles.get(i).setPos(i * 10, 0, 0);
        }
    }

    public void snowEffect2()
    {
        float elapsedTime = ElapsedTime.getInstance().getElapsedTime();

        for(int i = 0; i < MAX_PARTICLES; i++)
        {
            //Get a random number to see if a particle will be made visible if not already
            randomFloat = generator.nextFloat() * 360;

            if(particles.get(i).isVisible() || randomFloat < 2)
            {
                //If the particle is not visible, give it this random position
                if(!particles.get(i).isVisible())
                {
                    randomFloat = generator.nextFloat() * 2 * maxMinXEmitterSize;
                    partX[i] = randomFloat - maxMinXEmitterSize + pos.x;
                    randomFloat = generator.nextFloat() * 2 * maxMinXEmitterSize;
                    partZ[i] = randomFloat - maxMinZEmitterSize + pos.z;

                    particles.get(i).setPos(partX[i], pos.y, partZ[i]);
                }
                else
                {
                    //Equations that map out the path of the particle
                    particles.get(i).incPos(elapsedTime * -0.01f, elapsedTime * -0.05f, 0);
                }

                //If the particle reaches a certain height, it becomes invisible and is reset
                //to the emitter's position.
                if(particles.get(i).getPos().y > -100)
                {
                    particles.get(i).visible(true);
                }
                else
                {
                    //If the particle surpasses the point of death, it will be made
                    //invisible and have it's position reset to that of the emitter
                    particles.get(i).visible(false);
                    particles.get(i).setPos(pos.x, pos.y, pos.z);
                }
            }
        }
    }

    /***************************************************************************
     * desc: Decides whether or not to renderB the sprite
     **************************************************************************/
    public void visible(boolean _visible)
    {
        visible = _visible;
    }

    /***************************************************************************
     * desc: Finds out if the sprite is visible
     **************************************************************************/
    public boolean isVisible()
    {
        return visible;
    }

    public void renderB(GL gl, Matrix matrix)
    {
        if(visible)
        {
            render(gl, matrix);

            for(int i = 0; i < MAX_PARTICLES; i++)
            {
                particlesB[i].render(gl, matrix);
            }
        }
    }
}
