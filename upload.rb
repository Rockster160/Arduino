# TODO:
# Should log each time a "ping" broadcast comes in. Attempt to reconnect if nothing in 30 secs

# esp btn bedroom

# puts `zsh -c 'echo \'He\''`
# `zsh -c "echo 'Sourcing...'; source ~/.zshrc; echo 'Uploading...'; esp espBtn"`

require "fileutils"
require "pry-rails"

# Consts for easier more sensible setting of pins
D0=:D0;D5=:D5;D6=:D6;D7=:D7;D8=:D8;TX=:TX;RX=:RX;D1=:D1;D2=:D2;D3=:D3;D4=:D4

class Btn
  def self.channel(set_channel) = @channel = set_channel
  def self.wifi(set_wifi) = @wifi = set_wifi
  def self.led(pins) = @ledpins = pins
  def self.btns(pin_data) = @btndata = pin_data

  def self.data
    {
      channel: @channel || self.name.downcase,
      wifi: "#{(@wifi || :upstairs).to_s.capitalize}Wifi", # basement | upstairs
      led: @ledpins || [D0, D7, D8],
      btns: @btndata || { blu: D4, org: D6, yel: D3, red: D2, wht: D1, grn: D5 },
    }
  end
end

# ==================================================================================================

class Demo < Btn
  # channel :ring
end

class Desk < Btn
  channel :desk # optional - set by default (to name of class)
  wifi :upstairs # optional - set by default
  led [D0, D7, D8] # optional - set by default
  btns(
    busp:    D4,
    water:   D3,
    soda:    D2,
    protein: D1,
  )
end

class Pullups < Btn
  led [D3, D2, D1]
  btns pullups: D4
end

class Teeth < Btn
  led [D3, D2, D1]
  btns teeth: D4
end

class Laundry < Btn
  led [D4, D3, D2]
  btns laundry: D1
end

class Shower < Btn
  led [D5, D2, D1]
  btns shower: D4
end

class Bedroom < Btn
  led [D3, D2, D1]
  btns ignore: D4
end

# ==================================================================================================

def fail!(msg)
  puts "\e[31m#{msg}\e[0m"
  exit
end

def replace(oldstr, newstr)
  found = false
  if oldstr.end_with?("{")
    idx = @raw.index(oldstr)
    unless idx.nil?
      found = true
      endidx = @raw[idx..].index("};")
      @raw[idx..idx+endidx] = newstr
    end
  else
    @raw = @raw.map { |line|
      if line == oldstr
        found = true
        newstr
      else
        line
      end
    }
  end

  fail!("Unable to replace string- not found: '#{oldstr}'") unless found
end

src = "/Users/rocco/Documents/Arduino/espBtn/espBtn.ino"
backup = "/Users/rocco/Documents/Arduino/espBtn/espBtn.backup"

begin
  klass = Object.const_get(ARGV[1].capitalize)
rescue NameError
  fail!("No type found for '#{ARGV[1]}'")
end

# Raise if "uploading" comment is found in src
@raw = File.read(src).split("\n")

fail!("File is in incomplete state. May need to restore backup.") if @raw.first == "// uploading..."

# Create backup of original file
FileUtils.cp(src, backup)
# Add `uploading` code so that we know the new file is modified
@raw.prepend("// uploading...")

# Go through and replace with the new data
data = klass.data
replace(
  "const String channelId = \"demo\";",
  "const String channelId = \"#{data[:channel]}\";",
)
replace(
  "const int wifi = UpstairsWifi; // BasementWifi UpstairsWifi",
  "const int wifi = #{data[:wifi]};"
)
replace(
  "const int btnCount = 6;",
  "const int btnCount = #{data[:btns].length};"
)
replace(
  "const String btnIds[btnCount] = {",
  "const String btnIds[btnCount] = { #{data[:btns].keys.map { |v| "\"#{v}\"" }.join(", ")} };"
)
replace(
  "const int btns[btnCount] = {",
  "const int btns[btnCount] = { #{data[:btns].values.join(", ")} };"
)
replace(
  "const int colorPins[3] = { D0, D7, D8 };",
  "const int colorPins[3] = { #{data[:led].join(", ")} };"
)

# Save the modified file
File.write(src, @raw.join("\n"))
# Upload to the ESP
puts "\e[33m[LOGIT] | Uploading....\e[0m"
`zsh -c 'source ~/.zshrc && esp espBtn nomonitor'`
puts "\e[33m[LOGIT] | Upload Complete\e[0m"
# Restore the original file
File.write(src, File.read(backup))
