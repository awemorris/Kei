# q590 internal media value validation correction / 2026-10-02

Full review of css/media.c found two reproduced input-validation defects:
ordinary/ratio/discrete values ignored trailing tokens, and an arbitrary dimension
such as resolution:1px acted as pixel density. Ten ordinary cases added to the
already reviewed host-color-media corpus demonstrate six incorrect matches
against actual fdfbe58af engine objects:33/39, exit1. Four controls exercise real
dppx/x/dpi/dpcm units. Checkpoint28 retains exact patch/commands/raw results.

Primary [Media Queries4 syntax](https://www.w3.org/TR/mediaqueries-4/#mq-syntax)
and [CSS Values4 resolution units](https://www.w3.org/TR/css-values-4/#resolution)
checked2026-10-02 support complete value grammar and actual resolution units.
The correction requires values to consume their complete token sequence and
explicitly recognizes density units instead of treating arbitrary units as dppx.
The fixture also moves converted-source release after its immediate failure
check. Existing valid queries/units/conversions, current feature registry and
viewport model remain; no public API, foreign Phase, dependency, acceptance or
product/architecture decision change. Final native39/39 and page probe pass.

Supporting host-color-media.c already belongs to checkpoint10's reviewed209
inventory. Its full changed file is revalidated and hash/receipt updated without
counting it twice. Existing eight-test cap, unitless legacy resolution handling
and restricted condition grammar are retained limitations; this checkpoint does
not establish full Media Queries conformance or whole p172 clearance.
