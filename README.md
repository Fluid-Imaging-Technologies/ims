# ims

A Win7/Mil9/VS2010 project for experimenting with and testing the image processing chain 
for VisualSpreadsheet.

The ims program opens 8 windows you will want to monitor, so a setup with one big monitor
or two good size monitors makes ims much easier to use.

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


#### Window 2 - Control

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
in the Setup window. You will get three new windows. 

They are

#### Window 6 - Masked Image

The masked image is the result of subtracting the raw image from the background image.


#### Window 7 - Grayscale Image

The grayscale image is the result of image convolutions applied to the masked image.


#### Window 8 - Binarized Image

Here is where we use the threshold to finally decide which pixels to keep and which
to ignore. Pixels we keep are set to white and the rest set to black.


The VisualSpreadsheet image processing path is

## Raw -> Masked -> Grayscale -> Binary -> Overlay


You have the following control operations for each step.


### Raw Image Operations

These are all operations on the raw camera image which may be color. Use the "Raw..."
button to invoke any or all of the operations.

##### Raw Operations: Sharpen, Sharpen Aggressive, Smooth



### Masked Image Operations

The masked image is the result of subtracting the calibration image from the Raw Image.
The Background Elimination option allows for deciding which pixels that are different
from the calibration image to keep. The results are shown in the Mask Image as white
pixels (the difference) on a black background.

##### Background Elimination: Pixels Darker, Pixels Lighter, All Pixels Different


### Grayscale Image Operations

Grayscale operations are to the masked image. You can increase the contrast and/or
perform a number of convolutions.

The formula for calculating the contrast increase is

	new-pixel-value = pixel-value^n * (255.0 / 256.0^n)   
	where n = 0.9, 0.8, 0.7 or 0.6 and pixel values range from 0-255

##### Increase Contrast: None, 1(pow 9), 2(pow 8), 3(pow 7), 4(pow 6)

The grayscale convolutions can be strung together in any order, but each operation
can only be applied once in the current implementation. (This should probably
change.) The grayscale convolutions are configured from the dialog invoked from
the "Gray..." button.

##### Grayscale Convolutions: Close, Open, Prewitt, Prewitt+, Smooth, Sobel, Sobel+


### Binary Operations

The conversion from Grayscale to Binary uses the threshold value you specify
manually or if you check the auto threshold, the recommended threshold the
system calculates for you. The (Auto) field always calculates the value the
automatic thresholding would have used.

After conversion to grayscale to binary, there are some additional convolutions
that can be applied to the binary image. Access them using the "Binary..." button.

##### Binary Convolutions: Close, Open


You change any control setting and click "Apply" to see your changes.

The Save As buttons let you save your configurations so you can easily compare.
The Load buttons restore your saved configurations.
The Use Defaults button restores the control options to the VisualSpreadsheet
defaults.
