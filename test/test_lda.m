function test_lda(galen_file, output_folder)

% this files produces lda output from the GalenQt XML file
% however it does not read nested subfolders

if (exist('galen_file', 'var') == 0)
    [filename, pathname, filterindex] = uigetfile('*.xml', 'Choose the measurements file');
    if (filterindex == 0)
        fprintf('User cancelled\n');
        return
    end
    galen_file = fullfile(pathname, filename);
end
[filepath,name,ext] = fileparts(galen_file);


% read XML file to get the image and label data
root = parseXML(galen_file);
image_filenames = {};
labelled_points = {};
total_points = 0;
for h = 1: length(root)
    for i = 1: length(root(h).Children)
        child_level1 = root(h).Children(i);
        if (strcmp(child_level1.Name, 'ImagesFolder'))
            for hc1 = 1: length(child_level1.Children)
                child_level2 = child_level1.Children(hc1);
                if (strcmp(child_level2.Name, 'SingleChannelImage'))
                    image_filenames{end + 1} = fullfile(filepath, child_level2.Attributes('localPath'));
                end
            end
        end
        if (strcmp(child_level1.Name, 'LabelledPointsFolder'))
            for hc1 = 1: length(child_level1.Children)
                child_level2 = child_level1.Children(hc1);
                if (strcmp(child_level2.Name, 'LabelledPoints'))
                    points_list = {};
                    for hc2 = 1: length(child_level2.Children)
                        child_level3 = child_level2.Children(hc2);
                        if (strcmp(child_level3.Name, 'Point'))
                            points_list{end + 1} = [str2double(child_level3.Attributes('x')), str2double(child_level3.Attributes('y'))];
                            total_points = total_points + 1;
                        end
                    end
                    labelled_points{end + 1} = points_list;
                end
            end
        end
    end
end
    
inf = imfinfo(image_filenames{1});
cols = length(image_filenames);
image_data = {};
rows = inf.Width * inf.Height;
imageX = zeros(rows, cols);
for i = 1: length(image_filenames)
    fprintf('Reading "%s"\n', image_filenames{i});
    im = imread(image_filenames{i});
    image_data{i} = im;
    imageX(:, i) = im(:);
end

labels = [];
selected_data = {};
for i = 1: length(labelled_points)
    points_list = labelled_points{i};
    for j = 1: length(points_list)
        labels(end + 1) = i;
        point = points_list{j};
        image_values = zeros(1, length(image_filenames));
        for k = 1: cols
            current_image = image_data{k};
            image_values(k) = current_image(1 + inf.Height - floor(point(2) + 0.5), 1 + floor(point(1) + 0.5), 1);
        end
        selected_data{end + 1} = image_values;
    end
end

X = zeros(length(selected_data), cols);
for i = 1: length(selected_data)
    X(i, :) = selected_data{i};
end

no_dims = length(labelled_points) - 1;
[mappedX, mapping] = lda(X, labels, no_dims);
mapped_image_data = out_of_sample(imageX, mapping);

if (exist('output_folder', 'var') == 0)
    [folder_name] = uigetdir(filepath, 'Choose the destination folder');
    if (folder_name == 0)
        fprintf('User cancelled\n');
        return
    end
    output_folder = folder_name;
end

for i = 1: no_dims
    output_file = fullfile(output_folder, sprintf('LDA%d.tif', i));
    fprintf('Writing "%s"\n', output_file);
    timg = mapped_image_data(:, i);
    imwrite2tif(reshape(timg, inf.Height, inf.Width), [], output_file, 'single', 'Compression', Tiff.Compression.LZW);
end

function y = map_to_16_bit(x)
% convert an array into a 16 bit integer array mapping values to maximise the dynamic range
in_low = double(min(x(:)));
in_high = double(max(x(:)));
out_low = 0;
out_high = 256 * 256 - 1;
in_range = in_high - in_low;
out_range = out_high - out_low;
y = uint16(((double(x) - in_low) ./ in_range) .* out_range + out_low);
return

function [mappedX, mapping] = lda(X, labels, no_dims)
%LDA Perform the LDA algorithm
%
%   [mappedX, mapping] = lda(X, labels, no_dims)
%
% The function runs LDA on a set of datapoints X. The variable
% no_dims sets the number of dimensions of the feature points in the 
% embedded feature space (no_dims >= 1, default = 2). The maximum number 
% for no_dims is the number of classes in your data minus 1. 
% The function returns the coordinates of the low-dimensional data in 
% mappedX. Furthermore, it returns information on the mapping in mapping.
%
%

% This file is part of the Matlab Toolbox for Dimensionality Reduction.
% The toolbox can be obtained from http://homepage.tudelft.nl/19j49
% You are free to use, change, or redistribute this code in any way you
% want for non-commercial purposes. However, it is appreciated if you 
% maintain the name of the original author.
%
% (C) Laurens van der Maaten, Delft University of Technology


if ~exist('no_dims', 'var') || isempty(no_dims)
    no_dims = 2;
end

% Make sure data is zero mean
mapping.mean = mean(X, 1);
X = bsxfun(@minus, X, mapping.mean);

% Make sure labels are nice
[classes, bar, labels] = unique(labels);
nc = length(classes);

% Intialize Sw
Sw = zeros(size(X, 2), size(X, 2));

% Compute total covariance matrix
St = cov(X);

% Sum over classes
for i=1:nc
    
    % Get all instances with class i
    cur_X = X(labels == i,:);
    
    % Update within-class scatter
    C = cov(cur_X);
    p = size(cur_X, 1) / (length(labels) - 1);
    Sw = Sw + (p * C);
end

% Compute between class scatter
Sb = St - Sw;
Sb(isnan(Sb)) = 0; Sw(isnan(Sw)) = 0;
Sb(isinf(Sb)) = 0; Sw(isinf(Sw)) = 0;

% Make sure not to embed in too high dimension
if nc <= no_dims
    no_dims = nc - 1;
    warning(['Target dimensionality reduced to ' num2str(no_dims) '.']);
end

% Perform eigendecomposition of inv(Sw)*Sb
[M, lambda] = eig(Sb, Sw);

% Sort eigenvalues and eigenvectors in descending order
lambda(isnan(lambda)) = 0;
[lambda, ind] = sort(diag(lambda), 'descend');
M = M(:,ind(1:min([no_dims size(M, 2)])));

% Compute mapped data
mappedX = X * M;

% Store mapping for the out-of-sample extension
mapping.M = M;
mapping.val = lambda;

return

function t_point = out_of_sample(point, mapping)
t_point = bsxfun(@minus, point, mapping.mean) * mapping.M;
return

    
    
function imwrite2tif(varargin)
% IMWRITE2TIF Write image to tif file with specified datatype.
%   IMWRITE2TIF(IMGDATA,HEADER,IMFILE,DATATYPE) exports IMGDATA with HEADER
%   to TIF file named IMFILE. HEADER is usally obtained by IMFINFO from 
%   original image file, and it can also be left empty. String DATATYPE 
%   specifies data type for the export. Supported data types include 
%   logical, uint8, int8, uint16, int16, uint32, int32, uint64, int64, 
%   single and double.
%
%   IMWRITE2TIF(IMGDATA,HEADER,IMFILE,DATATYPE,TAG NAME1,TAG VALUE1,TAG NAME2,
%   TAG VALUE2,...) writes with specified Matlab supported TIF tag values.
%   These new tag values overide those already defined in HEADER.
%
%   Note 1: 
%       to avoid errors such as '??? Error using ==> tifflib The value for
%       MaxSampleValue must be ...', overide tag MaxSampleValue by Matlab
%       supported values. Or simply remove the tag from HEADER.
%
%   Note 2:
%       Overwriting of the existing image files is not checked. Be cautious
%       with the export image file name.
%
%   Example 1:
%       imgdata = imread('ngc6543a.jpg');
%       header  = imfinfo('ngc6543a.jpg');
%       imwrite2tif(imgdata,header,'new_ngc6543a.tif','uint8');
%
%   Example 2:
%       imgdata = imread('mri.tif');
%       imwrite2tif(imgdata,[],'new_mri.tif','int32','Copyright','MRI',
%       'Compression',1);
%
%   More information can be found by searching for 'Exporting Image Data 
%   and Metadata to TIFF Files' in Matlab Help.

%   Zhang Jiang 
%   $Revision: 1.4 $  $Date: 2018/06/17 19:45:26 $

if nargin<4 || mod(nargin,2)==1
    error('Invalid number of input arguments.');
end

% assign input argument
imgdata  = varargin{1};
header   = varargin{2};
imfile   = varargin{3};
datatype = varargin{4};
if nargin>4
    tag_name  = cell((nargin-4)/2,1);
    tag_value = cell((nargin-4)/2,1);
    for ii=1:(nargin-4)/2
        tag_name{ii}  = varargin{2*ii+3};
        tag_value{ii} = varargin{2*ii+4};
    end
end

% check errors
if ~isnumeric(imgdata)
     error('The first input argument (image data) must be a numeric array.');
end
if ~isempty(header) && ~isstruct(header)
    error('The second input argument (header info) must be empty or a structure.');
end
if ~ischar(imfile)
    error('The third input argument (output tif file name) must be string.');
end
switch lower(datatype)
    case 'logical'
        BitsPerSample = 1;
        SampleFormat  = 1;
        imgdata = logical(imgdata);
    case 'uint8'
        BitsPerSample = 8;   
        SampleFormat  = 1;         
        imgdata = uint8(imgdata);    
    case 'int8'
        BitsPerSample = 8;
        SampleFormat  = 2;       
        imgdata = int8(imgdata);          
    case 'uint16'
        BitsPerSample = 16;       
        SampleFormat  = 1;         
        imgdata = uint16(imgdata);           
    case 'int16'
        BitsPerSample = 16;       
        SampleFormat  = 2;         
        imgdata = int16(imgdata);        
    case 'uint32'
        BitsPerSample = 32;       
        SampleFormat  = 1;                
        imgdata = uint32(imgdata);            
    case 'int32'
        BitsPerSample = 32;       
        SampleFormat  = 2;     
        imgdata = int32(imgdata);         
     case 'single'
        BitsPerSample = 32;        
        SampleFormat  = 3;                
        imgdata = single(imgdata);       
    case {'uint64','int64','double'}
        BitsPerSample = 64;        
        SampleFormat  = 3;                
        imgdata = double(imgdata);           
    otherwise
        error('Invalid output data type.');
end

% creat a Tiff object
t = Tiff(imfile,'w');

% duplicate tags from header
tagnamelist = Tiff.getTagNames();
tagnamelist_delete = {...
    'StripByteCounts';...
    'StripOffsets';
    'TileByteCounts';...
    'TileOffsets';...
    'MaxSampleValue';...
    'MinSampleValue';...
    'ResolutionUnit'};
for ii=1:length(tagnamelist_delete)    % remove read only tag names
    tagnamelist(strcmpi(tagnamelist_delete{ii},tagnamelist)) = [];
end
if ~isempty(header)
    for ii=1:length(tagnamelist)
        if isfield(header,tagnamelist{ii}) && ~isempty(header.(tagnamelist{ii}))
           tagstruct.(tagnamelist{ii}) = header.(tagnamelist{ii});
        end
    end
end

% update tags determined from imgdata and datatype
tagstruct.ImageLength = size(imgdata,1);
tagstruct.ImageWidth = size(imgdata,2);
tagstruct.SamplesPerPixel = size(imgdata,3);
tagstruct.SampleFormat = SampleFormat;
tagstruct.BitsPerSample = BitsPerSample;

% update some default tags (these tags can be overriden by user input below)
tagstruct.Photometric = Tiff.Photometric.MinIsBlack;
tagstruct.Compression = 1;
tagstruct.PlanarConfiguration = Tiff.PlanarConfiguration.Chunky;
tagstruct.Software = 'MATLAB';

% update user input tag values
if nargin>4
    for ii=1:length(tag_name)
        tagstruct.(tag_name{ii}) = tag_value{ii};
    end
end

% set tags and write to tif file
t.setTag(tagstruct)
t.write(imgdata);
t.close();   
return

function theStruct = parseXML(filename)
% PARSEXML Convert XML file to a MATLAB structure.
try
   tree = xmlread(filename);
catch
   error('Failed to read XML file %s.',filename);
end

% Recurse over child nodes. This could run into problems 
% with very deeply nested trees.
try
   theStruct = parseChildNodes(tree);
catch
   error('Unable to parse XML file %s.',filename);
end


% ----- Subfunction PARSECHILDNODES -----
function children = parseChildNodes(theNode)
% Recurse over node children.
children = [];
if theNode.hasChildNodes
   childNodes = theNode.getChildNodes;
   numChildNodes = childNodes.getLength;
   allocCell = cell(1, numChildNodes);

   children = struct(             ...
      'Name', allocCell, 'Attributes', allocCell,    ...
      'Data', allocCell, 'Children', allocCell);

    for count = 1:numChildNodes
        theChild = childNodes.item(count-1);
        children(count) = makeStructFromNode(theChild);
    end
end

% ----- Subfunction MAKESTRUCTFROMNODE -----
function nodeStruct = makeStructFromNode(theNode)
% Create structure of node info.

nodeStruct = struct(                        ...
   'Name', char(theNode.getNodeName),       ...
   'Attributes', parseAttributes(theNode),  ...
   'Data', '',                              ...
   'Children', parseChildNodes(theNode));

if any(strcmp(methods(theNode), 'getData'))
   nodeStruct.Data = char(theNode.getData); 
else
   nodeStruct.Data = '';
end

% ----- Subfunction PARSEATTRIBUTES -----
function attributes = parseAttributes(theNode)
% Create attributes structure.

attributes = [];
if theNode.hasAttributes
   theAttributes = theNode.getAttributes;
   numAttributes = theAttributes.getLength;
   attributes = containers.Map;

   for count = 1:numAttributes
      attrib = theAttributes.item(count-1);
      attributes(char(attrib.getName)) = char(attrib.getValue);
   end
end

    