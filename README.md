# GalenQt

V1.0 released on 20th November 2018.

GalenQt is a software tool for organising and displaying multispectral image sets. It was created as part of an AHRC funded project "The Syriac Galen Palimpsest: Galen's On Simple Drugs and the Recovery of Lost Texts through Sophisticated Imaging Techniques" AH/M005704/1. It is written using Qt and released under the GPLv3 license. The software was written with the following golas in mind:

1. Provide a method of organising the multiple images associated with a folio, allowing rapid selection and display so that the user can find interesting features quickly.

2. Provide real time update for essential viewing controls such as brightness, contrast and gamma so that the user can customise the appearance of the image.

3. Provide a straightforward interface to allow the user to create pseudo-colour images by applying individual grey scale images to the red, green and blue colour channels

4. Provide an interactive method for generating training sets for supervised dimensional reduction algorithms

5. Provide an interface for choosing images, training sets, and applying a range of multi-spectral supervised and unsupervised learning techniques

GalenQt assumes that it is running on appropriate hardware for the size of images being analysed. Mutispectral images used in modern manuscript studies are typically 50-100 megapixel 16 bit grey scale with 20-40 different conditions (wavelengths/filters) used per page. Thus it is very easy for the images of a single page to require >16 GB of memory and we would recommend a machine with at least 32 GB to process high resolution images. It also benefits from a similarly high specification graphics card. GalenQt uses floating point image representations internally and custom OpenGL shaders to render these images efficiently. It therefore benefits greatly from a fast GPU with large amounts of memory. It was developed using Titan Xp GPU generously donated by the  NVIDIA Corporation which is an ideal specification for this purpose.

