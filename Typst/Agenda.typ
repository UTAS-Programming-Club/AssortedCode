// Uses Typst 0.14.2

// TODO: Set these
#let last_meeting_date = datetime(year: 0, month: 1, day: 1)
#let meeting_date = datetime(year: 0, month: 1, day: 1, hour: 0, minute: 0, second: 0)
#let wrap_meeting_date = true // Change to false if meeting is before 9 am
#let meeting_type = "Committee" // "Annual/Special General"
#let location = "on Discord (Online)" // "in Centenary 355"


// Import agenda layout
#import "/2025-2026/Templates/Structure-20260715.typ": *
#show: it => agenda(meeting_date, wrap_meeting_date, meeting_type, location, it)


= Introductory Items

== Attendance
Take attendance.\
Ensure attendance requirements are met.

== Apologies
Read out apologies.


= Minutes and Matters Arising

== Amendments for Last #simple_meeting_type(meeting_type) Meeting
// TODO: List any changes made to the minutes for the last meeting
Nil.

== Minutes for Last #simple_meeting_type(meeting_type) Meeting #appendix[A]
Date of relevant minutes: #get_formatted_date(last_meeting_date)

== Matters Arising
/* TODO: Add matters being continued from a previous meeting as follows
Some matter, see @some_matter\
⋮
Final matter, see @final_matter */
Nil.


= Correspondence

== Inward Correspondence #appendix[B]
// TODO: Use instructions in "Email Prep Instructions.txt" on Google Drive to generate the list
// Remove appendix heading here and in appendix section if there are none
Nil.

== Outward Correspondence #appendix[C]
// TODO: Use instructions in "Email Prep Instructions.txt" on Google Drive to generate the list
// Remove appendix heading here and in appendix section if there are none
Nil.


= Matters for Discussion

// TODO: Replace title and add any information required for a discussion during the meeting
== First Matter
Nil.

// TODO: Replace title and add any information required for a discussion during the meeting
== Some Matter <some_matter>
Nil.

// TODO: Add other matters as required

// TODO: Add small items to note with no major discussion or decisions expected here
== Small Items
- Item 1
- Item 2


= Matters for Noting

== President's Report
// TODO: Add report as required
Nil.

== Treasurer's Report
// TODO: Add report as required
Nil.

== Subcommittee and Other Reports (if applicable)
// TODO: Add report(s) as required
Nil.


#pagebreak()
#set heading(numbering: "A", supplement: "Appendix")
#counter(heading).update(0)

= Last #simple_meeting_type(meeting_type) Meeting Minutes
// TODO: Compress last meeting minutes with https://www.ilovepdf.com/compress_pdf and then upload to an "Appendix A" subfolder and embed as follows:
// #display_pdf("/2025-2026/2026MONTHDAY CM/Appendix A/2026MONTHDAY PC Committee Meeting Minutes.pdf", PAGECOUNT)

= Inward Correspondence
// TODO: Add drive folder public share link
Nil.

= Outward Correspondence
// TODO: Add drive folder public share link
Nil.
