# ims

A Win7/Mil9/VS2010 project for experimenting with and testing the image processing chain 
for VisualSpreadsheet.


##  Build

The project settings expect to find the Mil9 include and libraries in the following
relative location from this project

../FlowCAMSoftware/Mil9/

So the easiest thing to do is clone this project at the same level as FlowCAMSoftware
on your machine.


##  Usage

The ims program requires access to some rawfiles taken from a VisualSpreadsheet
run as well as a calibration image from the same run.

### Step 1 - File | Load Image

Load a rawfile_xxxxxx.tif from a VisualSpreadsheet run.

### Step 2 - File | Load Background Image

Load a cal_image_xxxxxx.tif image from the run.


At this point you will have 5 windows open on the screen.

#### Window 1 - IMS 1.3.0

The main IMS window that has a one line status bar that shows the elapsed
time for the last operation.


#### Window 2 - Setup

This is the control dialog for the app.


#### Window 3 - Raw: <some-path>/rawfile_xxxxxx.tif

This is the starting raw image to be processed.


#### Window 4 - Background: <some-path>/cal_image_xxxxxx.tif

The background calibration image for processing the raw image.


#### Window 5 - Overlay Results

This is the final result of the image processing. The resulting pixels that
will be considered part of this image will be shown in this window. The
4 buttons do the following:

##### Hide Pixels - Show or hide the pixels considered part of this image after processing.
This is the same as the binary image overlay in VisualSpreadsheet.

##### Hide Edges - This shows or hides the edges VisualSpreadsheet would calculate for this 
particle and that would be saved in the project edge file. Currently these edges are
calculated for all particles, but not used by VisualSpreadsheet for display or analysis.

##### Hide Rects - This shows or hides the bounding rectangle(s) for the particle that would
be used to distinguish this particle from others. The ims program does not do DNN.

##### Hide Image - This shows or hides the underlying raw image when showing the calculated
overlay data.


So now it's time to play. Without changing any settings click the Apply button
in the Setup window. You will get three new windows. I should mention that a two
monitor setup with big monitors makes ims a whole lot more fun to use.

The three new windows are

#### Window 6 - Masked Image

The masked image is the result of subtracting the raw image from the background image.


#### Window 7 - Grayscale Image

The grayscale image is the result of image convolutions applied to the masked image.


#### Window 8 - Binarized Image

Here is where we use the threshold to finally decide which pixels to keep and which
to ignore. Pixels we keep are set to white and the rest set to black.

