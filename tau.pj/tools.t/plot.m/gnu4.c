#include <stdio.h>
#include <plot.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
	int handle, i = 0, j;

	/* set Plotter parameters */		
	pl_parampl ("BITMAPSIZE", "300x150");
	pl_parampl ("VANISH_ON_DELETE", "yes");
	pl_parampl ("USE_DOUBLE_BUFFERING", "yes");

	/* create an X Plotter with the specified parameters */
	if ((handle = pl_newpl ("X", stdin, stdout, stderr)) < 0)
	{
		fprintf (stderr, "Couldn't create Plotter\n");
		return 1;
	}
	pl_selectpl (handle);			 /* select the Plotter for use */

	if (pl_openpl () < 0)			 /* open Plotter */
	{
		fprintf (stderr, "Couldn't open Plotter\n");
		return 1;
	}
	pl_space (0, 0, 299, 149);		/* specify user coordinate system */
	pl_linewidth (8);			/* line thickness in user coordinates */
	pl_filltype (1);			/* objects will be filled */
	pl_bgcolorname ("saddle brown");	/* background color for the window*/
	for (j = 0; j < 300; j++) {
		usleep(10000);
		pl_erase ();			/* erase window */
		pl_pencolorname ("red");	/* choose red pen, with cyan filling */
		pl_fillcolorname ("cyan");
		pl_ellipse (i, 75, 35, 50, i);	/* draw an ellipse */
		pl_colorname ("black");		/* choose black pen, with black filling */
		pl_circle (i, 75, 12);		/* draw a circle [the pupil] */
		i = (i + 2) % 300;		/* shift rightwards */
	}
	if (pl_closepl () < 0) {		 /* close Plotter */
		fprintf (stderr, "Couldn't close Plotter\n");
		return 1;
	}

	pl_selectpl (0);			/* select default Plotter */
	if (pl_deletepl (handle) < 0) {		/* delete Plotter we used */
		fprintf (stderr, "Couldn't delete Plotter\n");
		return 1;
	}
	return 0;
}

