;;StaffWithMarjs
(let ((tag "StaffWithMarks"))
(d-DirectivePut-score-prefix tag
"\\layout {
		  \\context {
			\\Score
			\\remove \"Mark_engraver\"
			}
		}
")
(if (d-Directive-staff? tag)
	(begin
		(d-DirectiveDelete-staff tag)
		(d-WarningDialog (_ "Marks removed from this staff. You may wish to put them on the top staff.")))
	(begin
		(d-DirectivePut-staff-override tag  (logior DENEMO_ALT_OVERRIDE  DENEMO_OVERRIDE_AFFIX  DENEMO_OVERRIDE_GRAPHIC)) 
		(SetDirectiveConditional "staff" tag)
		(d-DirectivePut-staff-prefix tag " \\consists \"Mark_engraver\" "))))
