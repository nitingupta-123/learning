
import java.io.*;

interface ICharacter{
    String getAbilities();
};

public class Mario implements ICharacter {
    @Override
    public String getAbilities() {
        return "Mario";
    }
}

public abstract class CharacterDecorator implements ICharacter {
    protected ICharacter character;

    public CharacterDecorator(ICharacter character) {
        this.character = character;
    }

    @Override
    public abstract String getAbilities();
}

class HeightPowerUpDecorator extends ICharacterDecorator{

    HeightPowerUpDecorator(ICharacter character){
        super(character);
    }

    public String getAbilities() {
        return character.getAbilities()  + " + Height Power";
    }
};

class GunPowerUpDecorator extends ICharacterDecorator{

    GunPowerUpDecorator(ICharacter character){
        super(character);
    }

    public String getAbilities() {
        return character.getAbilities()  + " + Gun Power";
    }

};

class Main{
    public static void main(String[] args) {

        ICharacter mario = new Mario();
        mario = HeightPowerUpDecorator(mario);
        System.out.println(mario.getAbilities());

        mario =GunPowerUpDecorator();
        System.out.println(mario.getAbilities());

    }
}