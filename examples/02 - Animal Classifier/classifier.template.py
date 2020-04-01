
<% define output(animal) %>
    print("A %s goes %s" % (<%% str | !lower($(animal).name) %%>, sound))
<% end %>

import sys

if len(sys.argv) < 2:
    print("Please provide an animal sound to classify.")
    exit(1)

sound = sys.argv[1]

<% for_each_record | "Animal" as ANIMAL %>

if sound == <%% str | $(ANIMAL).sound %%> :
    <% invoke output($(ANIMAL)) %>
    exit(0)

<% end %>

print("No animal makes that sound, as far as I can tell.")
