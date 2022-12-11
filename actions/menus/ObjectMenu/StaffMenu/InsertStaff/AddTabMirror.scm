;;; AddTabMirror 
(let ((staff (d-GetStaff)))
 (d-TabStaff)
 (d-AddAfter)
 (d-TabStaff)
 (d-AdditiveMirror (cons 'mix staff))
 (d-MoveToStaffUp)
 (d-TabStaff))