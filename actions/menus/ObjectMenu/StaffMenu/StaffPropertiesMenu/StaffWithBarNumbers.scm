;;StaffWithBarNumbers
(let ((tag "StaffWithBarNumbers"))
(d-DirectivePut-score-prefix tag
"\\layout {
		  \\context {
			\\Score
			\\remove \"Bar_number_engraver\"
			}
		}
")
(if (d-Directive-staff? tag)
	(begin
		(d-DirectiveDelete-staff tag)
		(d-WarningDialog (_ "Bar numbers removed from this staff. You may wish to put them on the top staff.")))
	(begin
		(d-DirectivePut-staff-override tag  (logior DENEMO_ALT_OVERRIDE  DENEMO_OVERRIDE_AFFIX  DENEMO_OVERRIDE_GRAPHIC)) 
		(SetDirectiveConditional "staff" tag)
		(d-DirectivePut-staff-prefix tag " \\consists \"Bar_number_engraver\" "))))
