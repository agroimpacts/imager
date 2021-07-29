from click.types import ParamType


class BboxType(ParamType):

    name = "bbox"

    def convert(self, value, param, ctx):
        # we're in the default case, so just return it
        if value == [0, 0, 0, 0]:
            return value

        try:
            if isinstance(value, list):
                num_strings = value
            else:
                num_strings = value.split(",")
            converted = [float(x) for x in num_strings]
            if len(converted) == 4:
                return converted
            else:
                self.fail(
                    f"Needed exactly four values for the bounding box. Got {len(converted)}."
                )
        except ValueError as e:
            self.fail(
                f"Could not read a bbox from the string for param {param}. Value: {value}: {e}"
            )


BBOX_TYPE = BboxType()
