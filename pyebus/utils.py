
def expand_img_info_tuple(img_info):
    """ Unpack the img_info tuple into a dictionary.
    See pyebus_main.cpp::getPvImageInfo() for the list of fields """
    keys = [
                "Width",
                "Height",
                "BitsPerPixel",
                "RequiredSize",
                "ImageSize",
                "EffectiveImageSize",
                "OffsetX",
                "OffsetY",
                "PaddingX",
                "PaddingY",
                "PixelSize",
                "BitsPerComponent",
                "IsPixelColor",
                "IsPixelHighRes",
                "IsPartialLineMissing",
                "IsFullLineMissing",
                "IsEOFByLineCount",
                "IsInterlacedEven",
                "IsInterlacedOdd",
                "IsImageDropped",
                "IsDataOverrun",]
    return {k: v for k, v in zip(keys, img_info)}
